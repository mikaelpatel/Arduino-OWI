#include "OWI.h"
#include "GPIO.h"
#include "Software/OWI.h"

#if defined(ARDUINO_attiny)
#include "Software/Serial.h"
Software::Serial<BOARD::D0> Serial;
Software::OWI<BOARD::D1> owi;
#else
Software::OWI<BOARD::D7> owi;
#endif

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
    int pos = Serial.print(i++);
    pos += Serial.print(':');
    for (size_t i = 0; i < sizeof(rom); i++)
      for (uint8_t mask = 0x80; mask != 0; mask >>= 1)
	Serial.print((rom[i] & mask) != 0);
    Serial.println();
    for (int i = 0; i < last - 1 + pos; i++) Serial.print('-');
    Serial.println('*');
  } while (last != owi.LAST);
  Serial.println();

  delay(5000);
}
