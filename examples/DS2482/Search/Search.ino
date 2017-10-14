#include "GPIO.h"
#include "TWI.h"
#include "Hardware/TWI.h"
#include "OWI.h"
#include "Hardware/OWI.h"
#include "assert.h"

Hardware::TWI twi;
Hardware::OWI owi(twi);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  // Scan one-wire bus and print rom code for all detected devices
  // on binary format and indicate discrepancy position.
  uint8_t rom[owi.ROM_MAX] = { 0 };
  int8_t last = owi.FIRST;
  int i = 0;
  do {
    last = owi.search_rom(0, rom, last);
    if (last == owi.ERROR) break;
    Serial.print(i++);
    Serial.print(':');
    int8_t pos = 0;
    for (size_t i = 0; i < sizeof(rom); i++)
      for (uint8_t mask = 0x80; mask != 0; mask >>= 1, pos++) {
	Serial.print((rom[i] & mask) != 0);
	if (pos == last) Serial.print('*');
      }
    if (pos == last) Serial.print('*');
    Serial.println();
  } while (last != owi.LAST);
  Serial.println();

  delay(5000);
}
