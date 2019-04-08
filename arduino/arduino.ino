#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht_nonblocking.h>
#include <PMS.h>
#include <SPI.h>
#include <SD.h>
#include <DS3231.h>
#include <MQ7.h>
#include <TimeLib.h>

SoftwareSerial debugSerial = SoftwareSerial(8, 9);

bool isBlink = true;
uint32_t lastBlink = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DS3231  rtc(SDA, SCL);

#define PACKET_SIZE 24
#define PACKET_ESP_SIZE 8
uint8_t dataBuffer[PACKET_ESP_SIZE];
uint8_t dataByteCount = 0;
uint32_t lastSendData = 0;

#define DHT_SENSOR_TYPE DHT_TYPE_22
#define DHT_SENSOR_PIN 7
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );
uint32_t lastReadDHT = 0;
float temperatureSum = 0;
float humiditySum = 0;
float temperature;
float humidity;
uint8_t dataDHTCount = 0;

SoftwareSerial dustSerial =  SoftwareSerial(4, 5); // RX, TX
PMS pms(dustSerial);
PMS::DATA data;
uint32_t startTimeGetDust = 0;
bool isGetDustData = false;
uint32_t pm1Sum = 0;
uint32_t pm25Sum = 0;
uint32_t pm10Sum = 0;
uint8_t dataDustCount = 0;

MQ7 mq7(A0, 5.0);
uint32_t lastReadMQ7 = 0;
uint32_t COppmSum = 0;
uint8_t dataMQ7Count = 0;

bool isSD = false;

void sendData2ESP();
void getDustData();
void getDHTdata();
void getMQ7data();
void lcdInit();
bool rtcAlarm();
void rtcUpdate();
void logDataToSD(float _temperature, float _humidity, float _pm1, float _pm25, float _pm10, float _COppm);
void rgbInit(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN)
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
}

void rgbShow(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN, int Value)
{
  int redPWM;
  int greenPWM;
  int bluePWM;

  if (Value < 12) {
    redPWM = 0;  // Green (0-255-0)
    greenPWM = 255;
    bluePWM = 0;
  }
  else if (Value >= 12 && Value < 35) {
    redPWM = 255;  //Yellow (255-255-0)
    greenPWM = 255;
    bluePWM = 0;
  }
  else if (Value >= 35 && Value < 55) {
    redPWM = 255;  //Orange	(255-165-0)
    greenPWM = 165;
    bluePWM = 0;
  }
  else if (Value >= 55 && Value < 150) {
    redPWM = 255;  //Red	(255-0-0)
    greenPWM = 0;
    bluePWM = 0;
  }
  else if (Value >= 150 && Value < 250) {
    redPWM = 128;  //Purple (128-0-128)
    greenPWM = 0;
    bluePWM = 128;
  }
  else {
    redPWM = 128;  //Marron (128,0,0)
    greenPWM = 255;
    bluePWM = 0;
  }

  analogWrite(RED_PIN, redPWM);
  analogWrite(BLUE_PIN, bluePWM);
  analogWrite(GREEN_PIN, greenPWM);
}

void setup()
{
  debugSerial.begin(9600);
  dustSerial.begin(9600);
  Serial.begin(9600);
  rgbInit(A2, A1, A3);
  lcdInit();
  rtc.begin();
  if (SD.begin(10))
    isSD = true;
}

void loop()
{
  if (millis() - lastBlink > 1000)
  {
    lastBlink = millis();
    Time t3 = rtc.getTime();
    char hourChar[2];
    sprintf(hourChar, "%2d", t3.hour);
    lcd.setCursor(0, 0);
    lcd.print(hourChar);
    isBlink = !isBlink;
    if (isBlink)
      lcd.print(":");
    else
      lcd.print(" ");
    char minChar[2];
    sprintf(minChar, "%2d", t3.min);
    lcd.setCursor(3, 0);
    lcd.print(minChar);
  }
  if (rtcAlarm())
  {
    sendData2ESP();
  }
  else if (isGetDustData)
  {
    if (millis() - startTimeGetDust < 2000)
      getDustData();
    else
      isGetDustData = false;
  }
  else if (Serial.available() > 0)
  {
    //read dust, mes, time
    dataBuffer[dataByteCount] = Serial.read();
    if (dataBuffer[0] == 66)
      dataByteCount++;
    if (dataByteCount == PACKET_ESP_SIZE)
    {
      dataByteCount = 0;
      uint16_t check = 0;
      for (uint8_t i = 0; i < PACKET_ESP_SIZE - 2; i++)
      {
        check += dataBuffer[i];
      }
      uint16_t check2 = (dataBuffer[PACKET_ESP_SIZE - 2] << 8) + dataBuffer[PACKET_ESP_SIZE - 1];
      if (check == check2)
      {
        if (dataBuffer[1] == 88)
        {
          //time
          rtcUpdate();
        }
        else if (dataBuffer[1] == 99)
        {
          //mes
        }

        //read dust
        isGetDustData = true;
        startTimeGetDust = millis();
      }
    }
  }
  else
  {
    getDHTdata();
    getMQ7data();
  }
}

void logDataToSD(float _temperature, float _humidity, float _pm1, float _pm25, float _pm10, float _COppm)
{
  if (isSD)
  {
    char fileName[12];
    Time t = rtc.getTime();

    sprintf(fileName, "%d-%d-%d.txt", t.date, t.mon, t.year - 2000);

    debugSerial.println(fileName);

    File f = SD.open(fileName, FILE_WRITE);

    if (f)
    {
      f.print(t.year);
      f.print("-");
      f.print(t.mon);
      f.print("-");
      f.print(t.date);
      f.print(" ");

      f.print(t.hour);
      f.print(":");
      f.print(t.min);
      f.print(":");
      f.print(t.sec);
      f.print(",");

      f.print(_temperature);
      f.print(",");
      f.print(_humidity);
      f.print(",");

      f.print(_pm1);
      f.print(",");
      f.print(_pm25);
      f.print(",");
      f.print(_pm10);
      f.print(",");

      f.println(_COppm);

      f.close();
    }
  }
}

void rtcUpdate()
{
  uint32_t internetTime = ((uint32_t)dataBuffer[2] << 24) + ((uint32_t)dataBuffer[3] << 16) + ((uint32_t)dataBuffer[4] << 8) + dataBuffer[5];
  Time t = rtc.getTime();
  uint32_t arduinoTime = rtc.getUnixTime(t);
  debugSerial.println(internetTime);
  debugSerial.println(arduinoTime);
  tmElements_t t2;
  breakTime(internetTime, t2);
  if (internetTime > arduinoTime)
  {
    if (internetTime - arduinoTime > 3)
    {
      rtc.setDOW();
      rtc.setTime(t2.Hour, t2.Minute, t2.Second);
      rtc.setDate(t2.Day, t2.Month, t2.Year + 1970);
    }
  }
  else
  {
    if (arduinoTime - internetTime > 3)
    {
      rtc.setDOW();
      rtc.setTime(t2.Hour, t2.Minute, t2.Second);
      rtc.setDate(t2.Day, t2.Month, t2.Year + 1970);
    }
  }
}

bool rtcAlarm()
{
  if (millis() - lastSendData > 5000)
  {
    Time t = rtc.getTime();
    if (t.sec == 0)
    {
      lastSendData = millis();
      return true;
    }
    else
      return false;
  }
  else
    return false;
}

void getMQ7data()
{
  if (millis() - lastReadMQ7 > 1000)
  {
    lastReadMQ7 = millis();

    float COppm = mq7.getPPM();


    debugSerial.println( COppm, 1 );

    if (COppm > 20.0 && COppm < 2000.0)
    {
      COppmSum += COppm;
      dataMQ7Count++;
      //logDataToSD(0, 0, 0, 0, 0, COppm);
      uint16_t COppmInt = COppm + 0.5;
      lcd.setCursor(13, 1);
      lcd.print(COppmInt);
    }
  }
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
        lcd.setCursor(2, 1); //Colum-Row
        lcd.print(humidityInt);
        uint8_t temperatureInt = temperature + 0.5;
        lcd.setCursor(7, 1); //Colum-Row
        lcd.print(temperatureInt);

        //logDataToSD(temperature, humidity, 0, 0, 0, 0);
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
    temp = (float)temperatureSum / dataDHTCount;
    hum = (float)humiditySum / dataDHTCount;
    temperatureSum = 0;
    humiditySum = 0;
    dataDHTCount = 0;
  }
  float pm1 = 0;
  float pm25 = 0;
  float pm10 = 0;
  if (dataDustCount != 0)
  {
    pm1 = (float)pm1Sum / dataDustCount;
    pm25 = (float)pm25Sum / dataDustCount;
    pm10 = (float)pm10Sum / dataDustCount;
    pm1Sum = 0;
    pm25Sum = 0;
    pm10Sum = 0;
    dataDustCount = 0;
  }
  float co = 0;
  if (dataMQ7Count != 0)
  {
    co = (float)COppmSum / dataMQ7Count;
    COppmSum = 0;
    dataMQ7Count = 0;
  }

  //sd
  logDataToSD(temp, hum, pm1, pm25, pm10, co);

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
  uint16_t _co = co;

  uint8_t dot_temp = (temp - _temp) * 100;
  uint8_t dot_hum = (hum - _hum) * 100;
  uint8_t dot_pm1 = (pm1 - _pm1) * 100;
  uint8_t dot_pm25 = (pm25 - _pm25) * 100;
  uint8_t dot_pm10 = (pm10 - _pm10) * 100;
  uint8_t dot_co = (co - _co) * 100;

  Time t = rtc.getTime();
  uint32_t arduinoTime = rtc.getUnixTime(t);

  uint8_t dataToEspBuffer[PACKET_SIZE] = {66, 77, _temp, dot_temp, _hum, dot_hum, (_pm1 >> 8), (_pm1 & 0xff), dot_pm1, (_pm25 >> 8), (_pm25 & 0xff), dot_pm25, (_pm10 >> 8), (_pm10 & 0xff), dot_pm10, (_co >> 8), (_co & 0xff), dot_co, 0, 0, 0, 0, 0, 0};

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
    isGetDustData = false;

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

    rgbShow(A2, A1, A3, pm25Int);

    sprintf(pm25Char, "%4d", pm25Int);

    lcd.setCursor(12, 0);
    lcd.print(pm25Char);

    //lcd.print(" ");
    //lcd.print(dataDustCount);

    debugSerial.print(data.PM_AE_UG_2_5);
    debugSerial.print(">");
    debugSerial.println(pm25Int);

    //logDataToSD(0, 0, data.PM_AE_UG_1_0, data.PM_AE_UG_2_5, data.PM_AE_UG_10_0, 0);
  }
}

void lcdInit()
{
  lcd.begin();
  lcd.setCursor(0, 0);    lcd.print("--:-- PM2.5:----");
  lcd.setCursor(0, 1);   lcd.print("T:-- H:-- CO:---");

  //  lcd.setCursor(0, 0); //Colum-Row
  //  lcd.print("PM2.5:---- ug/m3");
  //  lcd.setCursor(0, 1); //Colum-Row
  //  lcd.print("H:----%");
  //  lcd.setCursor(8, 1); //Colum-Row
  //  lcd.print("T:----*C");
}
