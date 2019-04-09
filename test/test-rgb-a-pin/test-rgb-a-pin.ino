#define RED_PIN A0
#define GREEN_PIN A2
#define BLUE_PIN A3

uint32_t last = 0;

void setup()
{
  // put your setup code here, to run once:
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(A1, OUTPUT);
  digitalWrite(A1, HIGH);


}

void loop()
{

  //  digitalWrite(RED_PIN, LOW);
  //  digitalWrite(GREEN_PIN, HIGH);
  //  digitalWrite(BLUE_PIN, LOW);
  //  delay(1000);
  //
  //  digitalWrite(RED_PIN, LOW);
  //  digitalWrite(GREEN_PIN, LOW);
  //  digitalWrite(BLUE_PIN, LOW);
  //  delay(1000);
  //
  //  digitalWrite(RED_PIN, LOW);
  //  digitalWrite(GREEN_PIN, LOW);
  //  digitalWrite(BLUE_PIN, HIGH);
  //  delay(1000);

  digitalWrite(BLUE_PIN, HIGH);
  digitalWrite(RED_PIN, LOW);
  if (millis() - last > 10)
    last = millis();
  else if (millis() - last > 7)
  {
    digitalWrite(GREEN_PIN, LOW);
  }
  else
  {
    digitalWrite(GREEN_PIN, HIGH);
  }

}
