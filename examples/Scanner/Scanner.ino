#include "GPIO.h"
#include "OWI.h"

OWI<BOARD::D7> owi;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  uint8_t rom[owi.ROM_MAX] = { 0 };
  int8_t last = owi.FIRST;
  int id = 0;
  size_t i;

  // Scan one-wire bus and print rom code for all detected devices
  do {
    last = owi.search_rom(0, rom, last);
    if (last == owi.ERROR) break;
    Serial.print(id++);
    Serial.print(':');
    Serial.print(F("family="));
    Serial.print(rom[0], HEX);
    Serial.print(F(", s/n="));
    for (i = 1; i < sizeof(rom) - 1; i++) {
      Serial.print(rom[i], HEX);
      if (i < sizeof(rom) - 2) Serial.print(' ');
    }
    Serial.print(F(", crc="));
    Serial.println(rom[i], HEX);
  } while (last != owi.LAST);
  Serial.println();

  delay(5000);
}
