#ifndef AirSenseCommunication_h
#define AirSenseCommunication_h


#include "AirSenseStructDefine.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPtimeESP.h>
#include <SoftwareSerial.h>

#define PACKET_SIZE 26
#define PACKET_TIME_SIZE 10

const char mqttServerAddress[15] = "188.68.48.86";
const uint16_t mqttServerPort = 1883;

void initMqttClient(char* _topic, char* _espID, PubSubClient& _mqttClient);
void messageCreate(char* _mes, uint8_t* _flashData);

#endif
