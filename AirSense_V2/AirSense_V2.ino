#include "UnitCode.h"

volatile uint8_t TimeCount = 0;

volatile bool SendData2SD_Enable = 0;
volatile bool SendData2ESP_Enable = 0;
volatile bool ReadSleepDust_Enable = 0;
volatile bool WakeUpDust_Enable = 0;
volatile bool ReadDataDHT_Enable = 0;
volatile bool ReadDataCO_Enable = 0;
volatile bool ShowTime2LCD_Enable = 0;
volatile bool RequireTimeESP_Enable = 0;

void setup()
{
  StructInit(&PMS, &dht22, &CO, &TimeSend);

  Serial.begin(9600);//ESPSerial.begin(115200);
  SD.begin(10);
  dht.begin();
  RGB_Init(A2, A1, A3);
  lcd.begin();

  DUSTSerial.begin(9600);
  // Switch to passive mode.
  pms.passiveMode();
  // Default state after sensor power, but undefined after ESP restart e.g. by OTA flash, so we have to manually wake up the sensor for sure.
  // Some logs from bootloader is sent via Serial port to the sensor after power up. This can cause invalid first read or wake up so be patient and wait for next read cycle.
  pms.wakeUp();

  lcd.setCursor(0, 0);		lcd.print("--:-- PM2.5:----");
  lcd.setCursor(0, 1);		lcd.print("t:-- h:-- CO:---");

  RTCAlarmInit();
  Timer1Init();
  if (DEBUG) Serial.println("End Setup...");
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (RTC.alarm(ALARM_2) )
  {
    TimeCount = 0;
    GetAVERValue(RTC.get(), &PMS, &dht22, &CO, &TimeSend);
  }
  if (ShowTime2LCD_Enable == 1)
  {
    if (SendData2ESP_Enable == 1)
    {
      SendData2ESP(&PMS, &dht22, &CO, &TimeSend);
      SendData2ESP_Enable = 0;
    }

    if (SendData2SD_Enable == 1)
    {
      WriteData2SD(&PMS, &dht22, &CO, &TimeSend);
      SendData2SD_Enable = 0;
    }

    if (RequireTimeESP_Enable == 1)
    {
      SendRequireTime();
      GetTimeFromESP(RTC.get());
      RequireTimeESP_Enable = 0;
    }

    if (WakeUpDust_Enable == 1)
    {
      pms.wakeUp();
      WakeUpDust_Enable = 0;
    }

    if (ReadSleepDust_Enable == 1)
    {
      readData(&PMS);
      pms.sleep();
      ReadSleepDust_Enable = 0;
    }

    if (ReadDataCO_Enable == 1)
    {
      GetADCValue(&CO);
      ReadDataCO_Enable = 0;
    }

    if (ReadDataDHT_Enable == 1)
    {
      ReadDHT22Value(&dht22);
      ReadDataDHT_Enable = 0;
    }

    ShowTime2LCD(RTC.get());
    if (TimeCount % 2 == 1) {
      lcd.setCursor(2, 0);	lcd.print(" ");
    }
    else {
      lcd.setCursor(2, 0);
      lcd.print(":");
    }
    ShowTime2LCD_Enable = 0;
  }
}

ISR (TIMER1_OVF_vect) //timer 1s
{
  TCNT1 = 49911;

  TimeCount ++;

  if (TimeCount == 48) // 47
  {
    SendData2ESP_Enable = 1;
  }

  if (TimeCount == 54) // 50
  {
    SendData2SD_Enable = 1;
  }

  if (TimeCount == 12 || TimeCount == 25 || TimeCount == 38) // 40
  {
    RequireTimeESP_Enable = 1;
  }

  if (TimeCount == 10) // 22
  {
    ReadSleepDust_Enable = 1;
  }
  if (TimeCount == 40) // 52
  {
    WakeUpDust_Enable = 1;
  }

  if (TimeCount == 2) // 15
  {
    ReadDataCO_Enable = 1;
  }

  if (TimeCount == 6) // 10
  {
    ReadDataDHT_Enable = 1;
  }

  ShowTime2LCD_Enable = 1;
}
