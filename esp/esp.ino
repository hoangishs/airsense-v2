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
  DEBUG.begin(9600);
  DEBUG.setDebugOutput(true);
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT);
  DEBUG.println(" - no wifi");
  WiFi.mode(WIFI_STA);
  initMqttClient(topic, espID, mqttClient);
  SPIFFS.begin();
  initQueueFlash(fileName);
  DEBUG.println(" - setup done");
}

void loop()
{
  if (longPressButton())
  {
    DEBUG.println(" - long press!");
    if (WiFi.beginSmartConfig())
    {
      DEBUG.println(" -------- start config wifi");
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
        uint16_t check2 = dataBuffer[PACKET_SIZE - 2] * 256 + dataBuffer[PACKET_SIZE - 1];
        if (check == check2)
        {
          lastGetData = millis();
          if (dateTime.valid)
          {
            //chuan bi data
            uint8_t saveData[FLASH_DATA_SIZE] = {0};
            for (uint8_t i = 0; i < PACKET_SIZE - 4; i++)
            {
              saveData[i] = dataBuffer[i + 2];
            }
            uint32_t nowSecond = dateTime.epochTime;
            if (lastGetTime > lastGetData)
              nowSecond = nowSecond - ((lastGetTime - lastGetData) / 1000);
            else
              nowSecond = nowSecond + ((lastGetData - lastGetTime) / 1000);
            for (uint8_t i = 0; i < 4; i++)
            {
              saveData[FLASH_DATA_SIZE - i - 1] = nowSecond & 0xff;
              nowSecond = nowSecond >> 8;
            }
            //enQueueFlash
            enQueueFlash(saveData, fileName);
            if (isTimeOK == false)
            {
              isFixTimeError = true;
            }
          }
          else
          {
            //chuan bi data
            uint8_t saveData[FLASH_DATA_SIZE] = {0};
            for (uint8_t i = 0; i < PACKET_SIZE - 4; i++)
            {
              saveData[i] = dataBuffer[i + 2];
            }
            //tinh thoi gian tu arduino
            uint32_t arduinoTime = ((saveData[8] << 24) + (saveData[9] << 16) + (saveData[10] << 8) + saveData[11]);
            //tinh thoi gian tu ban tin truoc
            //lay ban tin truoc
            //getPreData
            uint8_t preData[FLASH_DATA_SIZE] = {0};
            if (getPreData(preData, fileName))
            {
              uint32_t preArduinoTime = ((preData[8] << 24) + (preData[9] << 16) + (preData[10] << 8) + preData[11]);
              uint32_t preInternetTime = ((preData[12] << 24) + (preData[13] << 16) + (preData[14] << 8) + preData[15]);

              if (arduinoTime > preArduinoTime)
              {
                if (preInternetTime != 0)
                {
                  uint32_t internetTime = preInternetTime + ((arduinoTime - preArduinoTime ) / 1000);
                  for (uint8_t i = 0; i < 4; i++)
                  {
                    saveData[FLASH_DATA_SIZE - i - 1] = internetTime & 0xff;
                    internetTime = internetTime >> 8;
                  }
                }
              }
              else
              {
                //mat dien roi
                //tam thoi bo data cu di
                //emptyQueue
                emptyQueue(fileName);
              }
            }
            //enQueueFlash
            enQueueFlash(saveData, fileName);
          }
        }
      }
    }
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    if (isGetTime)
    {
      dateTime = NTPch.getNTPtime(7.0, 0);
      if (dateTime.valid)
      {
        //if got time
        lastGetTime = millis();
        isGetTime = false;
      }
    }
    else if (isTimeOK && (isQueueFlashEmpty(fileName) == false))
    {
      //if queue is not empty, publish data to server
      if (mqttClient.connected())
      {
        DEBUG.print(" - publish:.. ");
        uint8_t flashData[FLASH_DATA_SIZE];

        if (checkQueueFlash(flashData, fileName))
        {
          char mes[256] = {0};
          uint32_t epochTime = ((flashData[12] << 24) + (flashData[13] << 16) + (flashData[14] << 8) + flashData[15]);
          if (epochTime == 0)
          {
            isTimeOK = false;
          }
          else
          {
            float pm25Float = (flashData[4] << 8) + flashData[5];
            pm25Float = 1.33 * pow(pm25Float, 0.85);
            int pm25Int = pm25Float + 0.5;
            sprintf(mes, "{\"data\":{\"tem\":\"%d\",\"humi\":\"%d\",\"pm1\":\"%d\",\"pm2p5\":\"%d\",\"pm10\":\"%d\",\"CO\":\"%d\",\"time\":\"%d\"}}", flashData[0], flashData[1], ((flashData[2] << 8) + flashData[3]), ((flashData[4] << 8) + flashData[5]), ((flashData[6] << 8) + flashData[7]), pm25Int, epochTime);
            if (mqttClient.publish(topic, mes, true))
            {
              DEBUG.println(mes);
              deQueueFlash(fileName);
            }
            mqttClient.loop();
          }
        }
      }
      else if (millis() - lastMqttReconnect > 1000)
      {
        lastMqttReconnect = millis();
        DEBUG.println(" - mqtt reconnect ");
        mqttClient.connect(espID);
      }
    }
  }
  if (isFixTimeError)
  {
    //ban tin o dau queue ko co internetTime
    //ban tin o cuoi queue co internetTime
    DEBUG.println(mark);
    if (fixTimeError(&mark, fileName))
    {
      mark = 0;
      isTimeOK = true;
      isFixTimeError = false;
    }
  }
}
