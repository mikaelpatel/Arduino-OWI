#include "OWI.h"
#include "GPIO.h"

// Configure: Software/Hardware OWI Bus Manager
#define USE_SOFTWARE_OWI
#if defined(USE_SOFTWARE_OWI)
#include "Software/OWI.h"
Software::OWI<BOARD::D7> owi;

#else
#include "Hardware/OWI.h"
// Configure: Software/Hardware TWI Bus Manager
// #define USE_SOFTWARE_TWI
#if defined(USE_SOFTWARE_TWI)
#include "Software/TWI.h"
Software::TWI<BOARD::D18,BOARD::D19> twi;
#else
#include "Hardware/TWI.h"
Hardware::TWI twi;
#endif
Hardware::OWI owi(twi);
#endif

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
