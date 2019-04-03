void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  uint8_t dataBuffer[8] = {0, 0, 92, 200, 200, 200, 0, 0};

  uint32_t internetTime = (uint32_t)dataBuffer[2] << 24;

  Serial.println(internetTime);

  uint32_t internetTime2 = (uint32_t)dataBuffer[3] <<16;
  //internetTime2=internetTime2<<8;
  Serial.println(internetTime2);

  internetTime = internetTime + internetTime2;

  Serial.println(internetTime);
}

void loop() {
  // put your main code here, to run repeatedly:

}
