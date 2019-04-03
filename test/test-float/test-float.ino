void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  uint8_t aSum = 11;
  uint8_t count = 5;
  float a = (float)aSum / count;
  uint8_t _a = a;
  uint8_t dot_a = (a - _a) * 100;

  Serial.println(a);
  Serial.println(_a);
  Serial.println(dot_a);
}

void loop() {
  // put your main code here, to run repeatedly:

}
