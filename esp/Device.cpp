#include "./Device.h"

uint32_t lastPressButton = 0;
uint32_t lastBlinkLed = 0;

bool ledState = true;

bool longPressButton()
{
  if (millis() - lastPressButton > 3000 && digitalRead(PIN_BUTTON) == 0)
  {
    return true;
  }
  else if (digitalRead(PIN_BUTTON) == 1)
  {
    lastPressButton = millis();
  }
  return false;
}

void blinkLed(uint32_t _milisecond)
{
  if (millis() - lastBlinkLed > _milisecond)
  {
    lastBlinkLed = millis();
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState);
  }
}
