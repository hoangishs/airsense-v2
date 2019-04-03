
#include <DS3231.h>

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);
uint32_t last = 0;

void setup()
{
  Serial.begin(9600);
  rtc.begin();

  // The following lines can be uncommented to set the date and time
  //rtc.setDOW(WEDNESDAY);     // Set Day-of-Week to SUNDAY
  //rtc.setTime(12, 0, 0);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(1, 1, 2014);   // Set the date to January 1st, 2014
}

void loop()
{
  // Send time
  Serial.println(rtc.getTimeStr());

  if (millis() - last > 20000)
  {
    last = millis();
    rtc.setTime(12, 0, 0);
  }
}
