#include "./Communication.h"
#include "./Device.h"
#include "./Memories.h"
#include <ESP8266Ping.h>

const char* fileName = "/airv2.txt";

uint8_t dataBuffer[PACKET_SIZE] = {0};
uint8_t dataByteCount = 0;

bool isGetTime = true;
bool isSendTimeToArduino = false;
bool isInternet = false;

uint32_t lastPing = 0;
uint32_t lastMqttReconnect = 0;
uint32_t lastRequestArduino = 0;
uint32_t lastGetTime = 0;

char topic[25];
char espID[10];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

NTPtime NTPch("ch.pool.ntp.org");///???
strDateTime dateTime;

WiFiServer debugServer(8888);
WiFiClient debugClient;

uint32_t mark = 0;

void setup()
{
  DEBUG.begin(9600);
  Serial.begin(9600);
  DEBUG.setDebugOutput(true);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_LED, OUTPUT);
  WiFi.mode(WIFI_STA);
  initMqttClient(topic, espID, mqttClient);
  SPIFFS.begin();
  initQueueFlash(fileName);
  debugServer.begin();
  DEBUG.println(" - setup done");
  if (debugClient) debugClient.println(" - setup done");
}

void loop()
{
  if (longPressButton())
  {
    DEBUG.println(" - long press!");
    if (debugClient) debugClient.println(" - long press!");
    digitalWrite(PIN_LED, HIGH);
    if (WiFi.beginSmartConfig())
    {
      DEBUG.println(" -------- start config wifi");
    }
  }
  if (millis() - lastRequestArduino > 2500)
  {
    lastRequestArduino = millis();
    if (false)
    {
      //message from server
    }
    else if (isSendTimeToArduino)
    {
      //internet time
      isSendTimeToArduino = false;

      uint8_t timeToArduinoBuffer[PACKET_ESP_SIZE] = {66, 88, 0, 0, 0, 0, 0, 0};

      uint32_t internetTime = dateTime.epochTime;
      timeToArduinoBuffer[5] = internetTime & 0xff;
      internetTime = internetTime >> 8;
      timeToArduinoBuffer[4] = internetTime & 0xff;
      internetTime = internetTime >> 8;
      timeToArduinoBuffer[3] = internetTime & 0xff;
      internetTime = internetTime >> 8;
      timeToArduinoBuffer[2] = internetTime & 0xff;

      uint16_t sum = 0;
      for (uint8_t i = 0; i < PACKET_ESP_SIZE - 2; i++)
      {
        sum += timeToArduinoBuffer[i];
      }
      timeToArduinoBuffer[PACKET_ESP_SIZE - 2] = sum >> 8;
      timeToArduinoBuffer[PACKET_ESP_SIZE - 1] = sum & 0xff;

      DEBUG.print(" - time: ");
      if (debugClient) debugClient.println("send time to arduino");
      for (uint8_t i = 0; i < PACKET_ESP_SIZE; i++)
      {
        Serial.write(timeToArduinoBuffer[i]);
        DEBUG.print(timeToArduinoBuffer[i]);
        DEBUG.print(" ");
        if (debugClient) debugClient.print(timeToArduinoBuffer[i]);
        if (debugClient) debugClient.print(" ");
      }
      DEBUG.println();
      if (debugClient) debugClient.println();

      if (debugClient) debugClient.print("lastGetTime: ");
      if (debugClient) debugClient.println(lastGetTime);
      if (debugClient) debugClient.print("lastRequestArduino: ");
      if (debugClient) debugClient.println(lastRequestArduino);

      if (debugClient) debugClient.print("time: ");
      if (debugClient) debugClient.println(millis());
      DEBUG.print("time: ");
      DEBUG.println(millis());
    }
    else
    {
      //read dust request
      uint8_t requestDustBuffer[PACKET_ESP_SIZE] = {66, 77, 0, 0, 0, 0, 0, 66 + 77};

      DEBUG.print(" - request dust: ");
      DEBUG.println(millis());
      if (debugClient) debugClient.print(" - request dust: ");
      if (debugClient) debugClient.println(millis());

      for (uint8_t i = 0; i < PACKET_ESP_SIZE; i++)
      {
        Serial.write(requestDustBuffer[i]);
        DEBUG.print(requestDustBuffer[i]);
        DEBUG.print(" ");
        if (debugClient) debugClient.print(requestDustBuffer[i]);
        if (debugClient) debugClient.print(" ");
      }
      DEBUG.println();
      if (debugClient) debugClient.println();
    }
  }
  if (Serial.available() > 0)
  {
    dataBuffer[dataByteCount] = Serial.read();
    if (debugClient) debugClient.print(dataBuffer[dataByteCount]);
    if (debugClient) debugClient.print(" ");
    if (dataBuffer[0] == 66)
      dataByteCount++;
    if (dataByteCount == PACKET_SIZE)
    {
      dataByteCount = 0;
      if (debugClient) debugClient.println();
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
          for (uint8_t i = 0; i < FLASH_DATA_SIZE; i++)
          {
            saveData[i] = dataBuffer[i + 2];
          }

          //enQueueFlash
          if (enQueueFlash(saveData, fileName))
            if (debugClient) debugClient.println("enQueueFlash");
          uint32_t front = 0;
          uint32_t rear = 0;
          if (viewFlash(&front, &rear, fileName))
          {
            if (debugClient) debugClient.print("front: ");
            if (debugClient) debugClient.println(front);
            if (debugClient) debugClient.print("rear: ");
            if (debugClient) debugClient.println(rear);
          }
        }
      }
    }
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(PIN_LED, LOW);

    if (!debugClient)
    {
      debugClient = debugServer.available();
      if (debugClient) debugClient.println(debugClient.localIP().toString());
    }

    if (millis() - lastPing > 10000)
    {
      lastPing = millis();
      if (Ping.ping("www.google.com", 1))
      {
        isInternet = true;
        if (debugClient) debugClient.println("internet ok");
      } else
      {
        isInternet = false;
        if (debugClient) debugClient.println("no internet");
      }
    }

    if (isInternet)
    {
      if (isGetTime || (millis() - lastGetTime > 60000))
      {
        dateTime = NTPch.getNTPtime(7.0, 0);
        if (dateTime.valid)
        {
          //if got time
          isGetTime = false;
          isSendTimeToArduino = true;
          lastGetTime = millis();

          if (debugClient) debugClient.print("time ok: ");
          if (debugClient) debugClient.print(dateTime.hour);
          if (debugClient) debugClient.print(":");
          if (debugClient) debugClient.print(dateTime.minute);
          if (debugClient) debugClient.print(":");
          if (debugClient) debugClient.println(dateTime.second);
        }
      }
      else if ((isQueueFlashEmpty(fileName) == false))
      {
        //if queue is not empty, publish data to server
        if (debugClient) debugClient.println(" - queue is not empty");
        if (mqttClient.connected())
        {
          DEBUG.print(" - publish:.. ");
          if (debugClient) debugClient.print(" - publish:.. ");
          uint8_t flashData[FLASH_DATA_SIZE];

          if (checkQueueFlash(flashData, fileName))
          {
            char mes[256] = {0};

            float tem = flashData[0] + ((float)flashData[1] / 100);
            float humi = flashData[2] + ((float)flashData[3] / 100);

            float pm1 = (flashData[4] << 8) + flashData[5] + ((float)flashData[6] / 100);
            float pm2p5 = (flashData[7] << 8) + flashData[8] + ((float)flashData[9] / 100);
            float pm10 = (flashData[10] << 8) + flashData[11] + ((float)flashData[12] / 100);

            float CO = (flashData[13] << 8) + flashData[14] + ((float)flashData[15] / 100);

            uint32_t epochTime = (flashData[16] << 24) + (flashData[17] << 16) + (flashData[18] << 8) + flashData[19];

            sprintf(mes, "{\"data\":{\"tem\":\"%g\",\"humi\":\"%g\",\"pm1\":\"%g\",\"pm2p5\":\"%g\",\"pm10\":\"%g\",\"CO\":\"%g\",\"time\":\"%d\"}}", tem, humi, pm1, pm2p5, pm10, CO, epochTime);

            if (mqttClient.publish(topic, mes, true))
            {
              DEBUG.println(mes);
              deQueueFlash(fileName);
              if (debugClient) debugClient.println(mes);
              uint32_t front = 0;
              uint32_t rear = 0;
              if (viewFlash(&front, &rear, fileName))
              {
                if (debugClient) debugClient.print("front: ");
                if (debugClient) debugClient.println(front);
                if (debugClient) debugClient.print("rear: ");
                if (debugClient) debugClient.println(rear);
              }
            }
            mqttClient.loop();
          }
        }
        else if (millis() - lastMqttReconnect > 1000)
        {
          lastMqttReconnect = millis();
          DEBUG.println(" - mqtt reconnect ");
          if (debugClient) debugClient.println(" - mqtt reconnect ");
          mqttClient.connect(espID);
        }
      }
    }
  }
}
