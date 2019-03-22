#include "AirSenseCommunication.h"
#include "AirSenseDevice.h"
#include "AirSenseMemories.h"

const char* fileName = "/hieu.txt";

uint8_t dataBuffer[PACKET_SIZE] = {0};
uint8_t dataByteCount = 0;

bool isGetTime = true;
bool isTimeOK = true;
bool isFixTimeError = false;

uint32_t lastGetTime = 0;
uint32_t lastGetData = 0;
uint32_t lastMqttReconnect = 0;

char topic[25];
char espID[10];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

NTPtime NTPch("ch.pool.ntp.org");///???
strDateTime dateTime;

uint32_t mark = 0;

void setup()
{
  Serial.begin(9600);
  if (DEBUG) Serial.setDebugOutput(true);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_LED, OUTPUT);
  WiFi.mode(WIFI_STA);
  initMqttClient(topic, espID, mqttClient);
  SPIFFS.begin();
  initQueueFlash(fileName);
  if (DEBUG) Serial.println(" - setup done");
}

void loop()
{
  if (longPressButton())
  {
    if (DEBUG) Serial.println(" - long press!");
    digitalWrite(PIN_LED, HIGH);
    if (WiFi.beginSmartConfig())
    {
      if (DEBUG) Serial.println(" -------- start config wifi");
    }
  }
  if (Serial.available() > 0)
  {
    dataBuffer[dataByteCount] = Serial.read();
    if (dataBuffer[0] == 66)
      dataByteCount++;
    if (dataByteCount == PACKET_SIZE)
    {
      dataByteCount = 0;
      if (dataBuffer[1] == 77)
      {
        uint16_t check = 0;
        for (uint8_t i = 0; i < PACKET_SIZE - 2; i++)
        {
          check += dataBuffer[i];
        }
        uint16_t check2 = (dataBuffer[PACKET_SIZE - 2] << 8) + dataBuffer[PACKET_SIZE - 1];
        if (check == check2)
        {
          //chuan bi data
          uint8_t saveData[FLASH_DATA_SIZE] = {0};
          for (uint8_t i = 0; i < PACKET_SIZE - 4; i++)
          {
            saveData[i] = dataBuffer[i + 8];
          }

          struct tm t;
          time_t epoch;

          t.tm_hour = dataBuffer[2];
          t.tm_min = dataBuffer[3];
          t.tm_sec = dataBuffer[4];
          t.tm_mday = dataBuffer[5];          // Day of the month
          t.tm_mon = dataBuffer[6] - 1;         // Month, 0 - jan
          t.tm_year = dataBuffer[7] + 100;
          t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
          
          epoch = mktime(&t);

          uint32_t realTime = epoch;

          for (uint8_t i = 0; i < 4; i++)
          {
            saveData[FLASH_DATA_SIZE - i - 1] = realTime & 0xff;
            realTime = realTime >> 8;
          }
          //enQueueFlash
          enQueueFlash(saveData, fileName);
        }
      }
      else if (dataBuffer[1] == 88)
      {
        isGetTime = true;
      }
    }
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(PIN_LED, LOW);
    if (isGetTime)
    {
      dateTime = NTPch.getNTPtime(7.0, 0);
      if (dateTime.valid)
      {
        //if got time
        uint8_t timeToArduinoBuffer[PACKET_TIME_SIZE] = {66, 88, dateTime.hour, dateTime.minute, dateTime.second, dateTime.day, dateTime.month, dateTime.year - 2000, 0, 0};
        uint16_t sum = 0;
        for (uint8_t i = 0; i < PACKET_TIME_SIZE - 2; i++)
        {
          sum += timeToArduinoBuffer[i];
        }
        timeToArduinoBuffer[PACKET_TIME_SIZE - 2] = sum >> 8;
        timeToArduinoBuffer[PACKET_TIME_SIZE - 1] = sum & 0xff;
        if (DEBUG) Serial.print(" - time: ");
        for (uint8_t i = 0; i < PACKET_TIME_SIZE; i++)
        {
          Serial.write(timeToArduinoBuffer[i]);
          if (DEBUG) Serial.print(timeToArduinoBuffer[i]);
          if (DEBUG) Serial.print(" ");
        }
        if (DEBUG) Serial.println();
        isGetTime = false;
      }
    }
    else if ((isQueueFlashEmpty(fileName) == false))
    {
      //if queue is not empty, publish data to server
      if (mqttClient.connected())
      {
        if (DEBUG) Serial.print(" - publish:.. ");
        uint8_t flashData[FLASH_DATA_SIZE];

        if (checkQueueFlash(flashData, fileName))
        {
          char mes[256] = {0};
          uint32_t epochTime = ((flashData[13] << 24) + (flashData[14] << 16) + (flashData[15] << 8) + flashData[16]);

          float pm25Float = (flashData[4] << 8) + flashData[5];
          pm25Float = 1.33 * pow(pm25Float, 0.85);
          int pm25Int = pm25Float + 0.5;
          sprintf(mes, "{\"data\":{\"tem\":\"%d.%d\",\"humi\":\"%d.%d\",\"pm1\":\"%d.%d\",\"pm2p5\":\"%d.%d\",\"pm10\":\"%d.%d\",\"CO\":\"%d.%d\",\"time\":\"%d\"}}", flashData[0], flashData[1], flashData[2], flashData[3], ((flashData[4] << 8) + flashData[5]), flashData[6], ((flashData[7] << 8) + flashData[8]), flashData[9], ((flashData[10] << 8) + flashData[11]), flashData[12], pm25Int, 0, epochTime);
          if (mqttClient.publish(topic, mes, true))
          {
            if (DEBUG) Serial.println(mes);
            deQueueFlash(fileName);
          }
          mqttClient.loop();
        }
      }
      else if (millis() - lastMqttReconnect > 1000)
      {
        lastMqttReconnect = millis();
        if (DEBUG) Serial.println(" - mqtt reconnect ");
        mqttClient.connect(espID);
      }
    }
  }
}
