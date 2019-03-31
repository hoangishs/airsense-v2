#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht_nonblocking.h>
#include <PMS.h>
#include <SPI.h>
#include <SD.h>
#include <DS3232RTC.h>

#define DHT_SENSOR_TYPE DHT_TYPE_22
#define DHT_SENSOR_PIN 7

#define PACKET_SIZE 26
#define PACKET_TIME_SIZE 10

#define TIME_TO_SEND_DATA 60000

DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial dustSerial =  SoftwareSerial(4, 5); // RX, TX
SoftwareSerial debugSerial = SoftwareSerial(8, 9);

PMS pms(dustSerial);
PMS::DATA data;

unsigned long lastReadDHT = 0;

unsigned long pm1Sum = 0;
unsigned long pm25Sum = 0;
unsigned long pm10Sum = 0;
float temperatureSum = 0;
float humiditySum = 0;
float temperature;
float humidity;
uint16_t dataDustCount = 0;
uint16_t dataDHTCount = 0;

void sendData2ESP();
void getDustData();
void getDHTdata();
void lcdInit();

bool isResetESP = true;

unsigned long lastSendData = 0;

void setup()
{
  Serial.begin(9600);
  debugSerial.begin(9600);
  dustSerial.begin(9600);
  pinMode(2, OUTPUT);//use to reset esp
  digitalWrite(2, LOW);//set esp reset pin to high
  lcdInit();
}

void loop()
{
  if (millis() - lastSendData > TIME_TO_SEND_DATA)
  {
    lastSendData = millis();
    sendData2ESP();
    isResetESP = true;
  }

  if (isResetESP && (millis() - lastSendData > (TIME_TO_SEND_DATA - 10000)))
  {
    isResetESP = false;
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
  }

  getDustData();
  getDHTdata();
}

void getDHTdata()
{
  if ( millis( ) - lastReadDHT > 2500 )
  {
    if ( dht_sensor.measure( &temperature, &humidity ) == true )
    {
      lastReadDHT = millis( );

      debugSerial.println( temperature, 1 );
      debugSerial.println( humidity, 1 );

      if (temperature != 0 && humidity != 0)
      {
        temperatureSum += temperature;
        humiditySum += humidity;
        dataDHTCount++;

        uint8_t humidityInt = humidity + 0.5;
        lcd.setCursor(4, 1); //Colum-Row
        lcd.print(humidityInt);
        uint8_t temperatureInt = temperature + 0.5;
        lcd.setCursor(12, 1); //Colum-Row
        lcd.print(temperatureInt);
      }
    }
  }
}

void sendData2ESP()
{
  // tinh trung binh
  float temp = 0;
  float hum = 0;
  if (dataDHTCount != 0)
  {
    temp = temperatureSum / dataDHTCount;
    hum = humiditySum / dataDHTCount;
    temperatureSum = 0;
    humiditySum = 0;
    dataDHTCount = 0;
  }
  float pm1 = 0;
  float pm25 = 0;
  float pm10 = 0;
  if (dataDustCount != 0)
  {
    pm1 = pm1Sum / dataDustCount;
    pm25 = pm25Sum / dataDustCount;
    pm10 = pm10Sum / dataDustCount;
    pm1Sum = 0;
    pm25Sum = 0;
    pm10Sum = 0;
    dataDustCount = 0;
  }

  // cong thuc cua airbeam
  //        pm1=0.66776*pow(pm1,1.1);
  //        pm25=1.33*pow(pm25,0.85);
  //        pm10=1.06*pm10;

  //chuan bi data de gui cho esp
  uint8_t _temp = temp;
  uint8_t _hum = hum;
  uint16_t _pm1 = pm1;
  uint16_t _pm25 = pm25;
  uint16_t _pm10 = pm10;

  uint8_t dot_temp = (_temp - temp) * 100;
  uint8_t dot_hum = (_hum - hum) * 100;
  uint8_t dot_pm1 = (_pm1 - pm1) * 100;
  uint8_t dot_pm25 = (_pm25 - pm25) * 100;
  uint8_t dot_pm10 = (_pm10 - pm10) * 100;

  uint32_t arduinoTime = millis();//rtc

  uint8_t dataToEspBuffer[PACKET_SIZE] = {66, 77, _temp, dot_temp, _hum, dot_hum, (_pm1 >> 8), (_pm1 & 0xff), dot_pm1, (_pm25 >> 8), (_pm25 & 0xff), dot_pm25, (_pm10 >> 8), (_pm10 & 0xff), dot_pm10, 0, 0, 0, 0, 0, 0};

  dataToEspBuffer[PACKET_SIZE - 3] = arduinoTime & 0xff;
  arduinoTime = arduinoTime >> 8;
  dataToEspBuffer[PACKET_SIZE - 4] = arduinoTime & 0xff;
  arduinoTime = arduinoTime >> 8;
  dataToEspBuffer[PACKET_SIZE - 5] = arduinoTime & 0xff;
  arduinoTime = arduinoTime >> 8;
  dataToEspBuffer[PACKET_SIZE - 6] = arduinoTime & 0xff;

  uint16_t sum = 0;
  for (uint8_t i = 0; i < PACKET_SIZE - 2; i++)
  {
    sum += dataToEspBuffer[i];
  }
  debugSerial.println(sum);

  dataToEspBuffer[PACKET_SIZE - 2] = (sum >> 8);
  dataToEspBuffer[PACKET_SIZE - 1] = (sum & 0xff);

  //gui cho esp
  for (uint8_t i = 0; i < PACKET_SIZE; i++)
  {
    Serial.write(dataToEspBuffer[i]);
    debugSerial.print(dataToEspBuffer[i]);
    debugSerial.print(" ");
  }
  debugSerial.println();
}

void getDustData()
{
  if (pms.read(data))
  {
    debugSerial.println(data.PM_AE_UG_1_0);
    debugSerial.println(data.PM_AE_UG_2_5);
    debugSerial.println(data.PM_AE_UG_10_0);

    pm1Sum += data.PM_AE_UG_1_0;
    pm25Sum += data.PM_AE_UG_2_5;
    pm10Sum += data.PM_AE_UG_10_0;
    dataDustCount++;

    char pm25Char[4];

    float pm25Float = data.PM_AE_UG_2_5;
    pm25Float = 1.33 * pow(pm25Float, 0.85); //cong thuc cua airbeam
    uint16_t pm25Int = pm25Float + 0.5;

    sprintf(pm25Char, "%4d", pm25Int);

    lcd.setCursor(6, 0);
    lcd.print(pm25Char);

    debugSerial.print(data.PM_AE_UG_2_5);
    debugSerial.print(">");
    debugSerial.println(pm25Int);
  }
}

void lcdInit()
{
  lcd.begin();
  lcd.setCursor(0, 0); //Colum-Row
  lcd.print("PM2.5:---- ug/m3");
  lcd.setCursor(0, 1); //Colum-Row
  lcd.print("Hum:--%");
  lcd.setCursor(8, 1); //Colum-Row
  lcd.print("Tem:--*C");
}
