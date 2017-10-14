#include "GPIO.h"
#include "OWI.h"
#include "TWI.h"
#include "Hardware/TWI.h"
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
  // Print family, serial number and cyclic redundancy check sum
  uint8_t rom[owi.ROM_MAX] = { 0 };
  int8_t last = owi.FIRST;
  int id = 0;

  do {
    last = owi.search_rom(0, rom, last);
    if (last == owi.ERROR) break;

    // Print sequence number
    Serial.print(id++);

    // Print family code
    Serial.print(F(":family="));
    Serial.print(rom[0], HEX);

    // Print serial number
    Serial.print(F(",sn="));
    size_t i = 1;
    do {
      if (rom[i] < 0x10) Serial.print(0);
      Serial.print(rom[i], HEX);
      i += 1;
    } while (i < owi.ROM_MAX - 1);

    // Print cyclic redundancy check sum
    Serial.print(F(",crc="));
    if (rom[i] < 0x10) Serial.print(0);
    Serial.println(rom[i], HEX);
  } while (last != owi.LAST);

  Serial.println();
  delay(5000);
}
