#include <ARF.h>

uint8_t arfDict[512];
ARF arf(arfDict, sizeof(arfDict));

int led = 13;

void setup()
{
  pinMode(led, OUTPUT);
}

int blink()
{
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);
}

void loop()
{
  arf.go(blink);
}
