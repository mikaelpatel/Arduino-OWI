#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"
#include "Driver/DS18B20.h"
#include "Software/Serial.h"
#include "assert.h"

Software::Serial<BOARD::D0> Serial;
Software::OWI<BOARD::D1> owi;
DS18B20 sensor(owi);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  // Broadcast a convert request to all thermometer sensors
  // Print list of sensors, rom code, and temperature

  if (!sensor.convert_request(true)) return;
  delay(sensor.conversion_time());

  int8_t last = owi.FIRST;
  uint8_t* rom = sensor.rom();
  int id = 0;
  do {
    // Search for the next digital thermometer
    last = owi.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == owi.ERROR) break;

    // Read the scratchpad with current temperature, tiggers, etc
    ASSERT(sensor.read_scratchpad(false));
    int8_t low, high;
    sensor.get_trigger(low, high);

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
    Serial.print(rom[i], HEX);

    // Print conversion resolution
    Serial.print(F(",resolution="));
    Serial.print(sensor.resolution());

    // Print alarm trigger threshols
    Serial.print(F(",trigger=["));
    Serial.print(low);
    Serial.print(F(".."));
    Serial.print(high);

    // And temperature
    Serial.print(F("],temperature="));
    Serial.println(sensor.temperature());
  } while (last != owi.LAST);

  Serial.println();
  delay(4000);
}
