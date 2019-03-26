//------------------------------------------------------------------------
#ifndef UnitCode_h
#define UnitCode_h

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <avr/interrupt.h>
#include <DHT.h>
#include <DS3232RTC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PMS.h>
//--------------------------------------------------------------------
SoftwareSerial DUSTSerial = SoftwareSerial(4, 5);

PMS pms(DUSTSerial);
PMS::DATA data;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define DEBUG true

//--------------------------------------------------------------------
typedef struct StructTime
{
  uint8_t 	Hour;
  uint8_t 	Minute;
  uint8_t 	Second;
  uint8_t 	Day;
  uint8_t 	Month;
  uint16_t 	Year;
} Time_t;

typedef struct StructPMS
{
  uint8_t 	DustCount;
  uint16_t 	PM1_0;
  uint16_t 	PM2_5;
  uint16_t	PM10_0;
  uint16_t 	PM1_0_Total;
  uint16_t 	PM2_5_Total;
  uint16_t 	PM10_0_Total;
} PMS_t;

typedef struct StructDHT22
{
  uint8_t 	DHT22Count;
  uint16_t 	TEMP;
  uint16_t	HUMI;
  uint16_t	TempTotal;
  uint16_t	HumiTotal;
} DHT22_t;

typedef struct StructCO
{
  uint8_t 	ADCcount;
  uint16_t	CO;
  uint16_t 	ADCTotal;
} CO_t;

PMS_t 	PMS;
DHT22_t dht22;
CO_t 	CO;
Time_t 	TimeSend;
//---------------------------------------------------------------------

/*=====================================================================*/
void Timer1Init(void); //Set Timer1 for 1s timer
void RGB_Init(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN);// LED RGB Init
void RTCAlarmInit(void);//Init Alarm 2 for every 1 minute
void StructInit(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d);
void GetADCValue(struct StructCO *p); //Get ADC value
void ReadDHT22Value(struct StructDHT22 *p); // Read dht22 Value
void GetAVERValue(time_t t, struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d);//Calculate the AVER value
void RGB_Show(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN, int Value);//Show LED color follow a value
void ShowTime2LCD(time_t t);//Show time to LCD 16x2
void SendRequireTime();//SendESP to require ESP get time from internet
void GetTimeFromESP(time_t t);//Get time ESP
void SendData2ESP(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d);
void WriteData2SD(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d); //Write_data to SD_card
/*=====================================================================*/

//---------------------------------------------------------------------
void StructInit(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d)
{
  d 	->	Hour				= 0;
  d 	->	Minute				= 0;
  d 	->  Second				= 0;
  d 	->	Day					= 1;
  d 	->	Month				= 1;
  d 	->	Year				= 19;

  a 	->	DustCount			= 0;
  a 	->	PM1_0				= 0;
  a 	->	PM2_5				= 0;
  a	-> 	PM10_0				= 0;
  a 	->	PM1_0_Total			= 0;
  a 	->	PM2_5_Total			= 0;
  a 	->	PM10_0_Total		= 0;

  b 	->	DHT22Count			= 0;
  b 	->	TEMP				= 0;
  b	->	HUMI				= 0;
  b	->	TempTotal			= 0;
  b	->	HumiTotal			= 0;

  c 	->	ADCcount			= 0;
  c	->	CO					= 0;
  c 	->	ADCTotal			= 0;
}

//---------------------------------------------------------------------
void Timer1Init()
{
  cli();                                  // close interrupt
  /* Reset Timer/Counter1 */
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;
  /* Setup Timer/Counter1 */
  TCCR1B |= (1 << CS12) | (1 << CS10);    // prescale = 1024
  TCNT1 = 49911;
  TIMSK1 = (1 << TOIE1);                  // Overflow interrupt enable
  sei();                                  // set enable interrupt
}

//---------------------------------------------------------------------
void RTCAlarmInit()
{
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);

  tmElements_t tm;
  tm.Hour = 00;           // set the RTC to an arbitrary time
  tm.Minute = 00;
  tm.Second = 00;
  tm.Day = 1;
  tm.Month = 1;
  tm.Year = 49;  // tmElements_t.Year is the offset from 1970
  RTC.write(tm);          // set the RTC from the tm structure

  // set Alarm 2 to occur once per minute
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);
  // clear the alarm flag
  RTC.alarm(ALARM_2);
}

//---------------------------------------------------------------------
void readData(struct StructPMS *p)
{
  // Clear buffer (removes potentially old data) before read. Some data could have been also sent before switching to passive mode.
  while (DUSTSerial.available()) {
    DUSTSerial.read();
  }

  pms.requestRead();

  if (pms.readUntil(data))
  {
    /* 	p -> PM1_0_Total += data.PM_AE_UG_1_0;
    	p -> PM2_5_Total += data.PM_AE_UG_2_5;
    	p -> PM10_0_Total += data.PM_AE_UG_10_0;
    	p -> DustCount ++; */

    p -> PM1_0 = data.PM_AE_UG_1_0;
    p -> PM2_5 = data.PM_AE_UG_2_5;
    p -> PM10_0 = data.PM_AE_UG_10_0;
    RGB_Show(A2, A1, A3, p -> PM2_5);
    lcd.setCursor(12, 0);	lcd.print("    ");
    lcd.setCursor(12, 0);	lcd.print(p -> PM2_5);
  }
}

//---------------------------------------------------------------------
void GetADCValue(struct StructCO *p)
{
  float ADCvalue = analogRead(A0);
  /* 	p -> ADCTotal += ADCvalue;
  	p -> ADCcount ++; */

  p -> CO = (ADCvalue * (1000 - 10)) / 1023;
  lcd.setCursor(13, 1);	lcd.print("   ");
  lcd.setCursor(13, 1);	lcd.print(p -> CO);
}

//---------------------------------------------------------------------
void ReadDHT22Value(struct StructDHT22 *p)
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humi = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temp = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humi) || isnan(temp)) {
    //  Serial.println("Failed to read from DHT sensor!");
    return;
  }

  /* 	p -> HumiTotal += humi*10;
  	p -> TempTotal += temp*10;
  	p -> DHT22Count ++; */

  p -> HUMI = humi * 10;
  p -> TEMP = temp * 10;
  lcd.setCursor(2, 1);		lcd.print(p -> TEMP / 10);
  lcd.setCursor(7, 1);		lcd.print(p -> HUMI / 10);
}

//---------------------------------------------------------------------
void GetAVERValue(time_t t, struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d)
{
  /* 	a -> PM1_0  = a -> PM1_0_Total  / a -> DustCount;
  	a -> PM2_5  = a -> PM2_5_Total  / a -> DustCount;
  	a -> PM10_0 = a -> PM10_0_Total / a -> DustCount;

  	b -> TEMP = b -> TempTotal / b -> DHT22Count;
  	b -> HUMI = b -> HumiTotal / b -> DHT22Count;

  	float ADC_Aver = c -> ADCTotal / c -> ADCcount;
  	c -> CO = (ADC_Aver*(1000-10))/1023; */

  d -> Day 	= day(t);
  d -> Month	= month(t);
  d -> Year	= year(t) - 2000;
  d -> Hour	= hour(t);
  d -> Minute	= minute(t);

  /*  	lcd.setCursor(11,0);	lcd.print("    ");
   	lcd.setCursor(11,0);	lcd.print(a -> PM2_5);

  	lcd.setCursor(2,1);		lcd.print(b -> TEMP/10);

  	lcd.setCursor(7,1);		lcd.print(b -> HUMI/10);

  	lcd.setCursor(13,1);	lcd.print("   ");
  	lcd.setCursor(13,1);	lcd.print(c -> CO);

  	RGB_Show(A2, A1, A3, a -> PM2_5);

   	a -> PM1_0_Total = a -> PM2_5_Total = a -> PM10_0_Total = 0;
  	c -> ADCTotal = b -> TempTotal = b -> HumiTotal = 0;
  	a -> DustCount = b -> DHT22Count = c-> ADCcount = 0; */
}

//---------------------------------------------------------------------
void RGB_Init(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN)
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
}

void RGB_Show(const char RED_PIN, const char GREEN_PIN, const char BLUE_PIN, int Value)
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

//---------------------------------------------------------------------
void ShowTime2LCD(time_t t)
{

  lcd.setCursor(0, 0);		lcd.print("  ");
  lcd.setCursor(0, 0);		lcd.print(hour(t));
  lcd.setCursor(3, 0);		lcd.print("  ");
  lcd.setCursor(3, 0);		lcd.print(minute(t));
}

//---------------------------------------------------------------------
void WriteData2SD(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d)
{
  char FileName[12];

  sprintf(FileName, "A1_%d%d%d.txt", d->Day, d->Month, d->Year);

  /* 	sprintf(dataString,"airsense_01\t%d/%d/%d-%d:%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d%d%d%d%d",
  		d->Day,d->Month,d->Year,d->Hour,d->Minute,b->TEMP,b->HUMI,a->PM1_0,a->PM2_5,a->PM10_0,c->CO,
  			 a->DustCount,b->DHT22Count,c->ADCcount,d->CorrectTime_State,d->LossConnect_State);
  */

  File myFile = SD.open(FileName, FILE_WRITE);
  if (myFile)
  {
    if (myFile.size() == 0)
    {
      myFile.println("NodeID\tTimeStamp\ttemp\thum\tPM1\tPM2.5\tPM10\tCO");
    }

    myFile.print("Airsense_01\t");
    myFile.print(d->Day);		myFile.print(d->Month);		myFile.print(d->Year);
    myFile.print("-");
    myFile.print(d->Hour);		myFile.print(d->Minute); 	myFile.print("\t");
    myFile.print(b->TEMP / 10); 	myFile.print('.');			myFile.print(b->TEMP % 10);
    myFile.print("\t");
    myFile.print(b->HUMI / 10); 	myFile.print('.');			myFile.print(b->HUMI % 10);
    myFile.print("\t");
    myFile.print(a->PM1_0); 	myFile.print("\t");
    myFile.print(a->PM2_5); 	myFile.print("\t");
    myFile.print(a->PM10_0); 	myFile.print("\t");
    myFile.println(c->CO);
    myFile.close();
  }
}

//---------------------------------------------------------------------
void SendData2ESP(struct StructPMS *a, struct StructDHT22 *b, struct StructCO *c, struct StructTime *d)
{
  //	66 77 SendHour SendMinute SendSecond SendDay SendMonth SendYear TEMPaver .TEMPaver RHaver .Rhaver PM1_0_Aver>>8 PM1_0_Aver&0xFF .pm1 PM2_5_Aver>>8 PM2_5_Aver&0xFF .pm2.5
  //	PM10_0_Aver>>8 PM10_0_Aver&0xFF .pm10 CO_value>>8 CO_value&0xFF .co sumESP>>8 sumESP&0xFF
  uint8_t Data2ESP_Buff[26] =	{66, 77, d->Hour, d->Minute, 0, d->Day, d->Month, d->Year,
                               b->TEMP / 10, b->TEMP % 10, b->HUMI / 10, b->HUMI % 10,
                               a->PM1_0 >> 8, a->PM1_0 & 0xFF, 0, a->PM2_5 >> 8, a->PM2_5 & 0xFF, 0, a->PM10_0 >> 8, a->PM10_0 & 0xFF, 0,
                               c->CO >> 8, 0, c->CO & 0xFF, 0, 0
                              };
  uint16_t Sum = 0;

  Sum = Data2ESP_Buff[0] + Data2ESP_Buff[1] + Data2ESP_Buff[2] + Data2ESP_Buff[3] + Data2ESP_Buff[4]
        + Data2ESP_Buff[5] + Data2ESP_Buff[6] + Data2ESP_Buff[7] + Data2ESP_Buff[8] + Data2ESP_Buff[9]
        + Data2ESP_Buff[10] + Data2ESP_Buff[11] + Data2ESP_Buff[12] + Data2ESP_Buff[13] + Data2ESP_Buff[14]
        + Data2ESP_Buff[15] + Data2ESP_Buff[16] + Data2ESP_Buff[17] + Data2ESP_Buff[18] + Data2ESP_Buff[19]
        + Data2ESP_Buff[20] + Data2ESP_Buff[21] + Data2ESP_Buff[22] + Data2ESP_Buff[23];

  Data2ESP_Buff[24] = Sum >> 8;
  Data2ESP_Buff[25] = Sum & 0xFF;

  Serial.write(Data2ESP_Buff[0]);		Serial.write(Data2ESP_Buff[1]);
  Serial.write(Data2ESP_Buff[2]);		Serial.write(Data2ESP_Buff[3]);
  Serial.write(Data2ESP_Buff[4]);		Serial.write(Data2ESP_Buff[5]);
  Serial.write(Data2ESP_Buff[6]);		Serial.write(Data2ESP_Buff[7]);
  Serial.write(Data2ESP_Buff[8]);		Serial.write(Data2ESP_Buff[9]);
  Serial.write(Data2ESP_Buff[10]);	Serial.write(Data2ESP_Buff[11]);
  Serial.write(Data2ESP_Buff[12]);	Serial.write(Data2ESP_Buff[13]);
  Serial.write(Data2ESP_Buff[14]);	Serial.write(Data2ESP_Buff[15]);
  Serial.write(Data2ESP_Buff[16]);	Serial.write(Data2ESP_Buff[17]);
  Serial.write(Data2ESP_Buff[18]);	Serial.write(Data2ESP_Buff[19]);
  Serial.write(Data2ESP_Buff[20]);	Serial.write(Data2ESP_Buff[21]);
  Serial.write(Data2ESP_Buff[22]);	Serial.write(Data2ESP_Buff[23]);
  Serial.write(Data2ESP_Buff[24]);	Serial.write(Data2ESP_Buff[25]);

  if (DEBUG) Serial.println(Data2ESP_Buff[0]);
  if (DEBUG) Serial.println(Data2ESP_Buff[1]);
  if (DEBUG) Serial.println(Data2ESP_Buff[2]);
  if (DEBUG) Serial.println(Data2ESP_Buff[3]);
  if (DEBUG) Serial.println(Data2ESP_Buff[4]);
  if (DEBUG) Serial.println(Data2ESP_Buff[5]);
  if (DEBUG) Serial.println(Data2ESP_Buff[6]);
  if (DEBUG) Serial.println(Data2ESP_Buff[7]);
  if (DEBUG) Serial.println(Data2ESP_Buff[8]);
}

void SendRequireTime()
{
  Serial.write(66);	Serial.write(88);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(0);
  Serial.write(0);	Serial.write(154);
}

void GetTimeFromESP(time_t t)
{
  uint8_t DataTime_i = 0;
  int sum = 0;
  int checksum = 0;
  int DataTime[10] = {0};
  uint32_t lastGetTime = millis();
  while (true)
  {
    if (millis() - lastGetTime > 700) break;

    if (Serial.available() > 0)
    {
      while (Serial.available())
      {
        DataTime [DataTime_i] = Serial.read();
        if (DataTime[0] == 66) DataTime_i++;
        if (DataTime_i == 10)
        {
          DataTime_i = 0;
          if (DataTime[1] == 88)
          {
            sum = DataTime[0] + DataTime[1] + DataTime[2] + DataTime[3] + DataTime[4] + DataTime[5] + DataTime[6] + DataTime[7];
            checksum = DataTime[8] * 256 + DataTime[9];
            if (sum == checksum)
            {
              if (minute(t) != DataTime[3])
              { // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
                tmElements_t tm;
                tm.Hour = DataTime[2];          // set the RTC to an arbitrary time
                tm.Minute = DataTime[3];
                tm.Second = DataTime[4];
                tm.Day = DataTime[5];
                tm.Month = DataTime[6];
                tm.Year = DataTime[7] + 30;  // tmElements_t.Year is the offset from 1970
                RTC.write(tm);          // set the RTC from the tm structure
              }
            }
          }
        }
      }
      break;
    }
  }
}

//---------------------------------------------------------------------
#endif
