#include <ESP8266WiFi.h>

uint32_t last = 0;

void setup()
{
  Serial.begin(9600);
}
void loop()
{
  if (millis() - last > 2500)
  {
    last = millis();

    uint8_t data_buff[10] = {66, 77, 0, 0, 0, 0, 0, 0, 0, 66 + 77};

    for (uint8_t i = 0; i < 10; i++)
    {
      Serial.write(data_buff[i]);
    }
  }
}
