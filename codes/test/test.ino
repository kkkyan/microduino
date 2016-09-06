#include <Adafruit_SSD1306.h>

#define  sensorPin  6

int state;

void setup()
{
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
}
void loop()
{
  state = digitalRead(sensorPin);
  Serial.println(state);
  if (state == 1)
    Serial.println("Somebody is in this area!");
  else
    Serial.println("No one!");
  delay(500);
}
