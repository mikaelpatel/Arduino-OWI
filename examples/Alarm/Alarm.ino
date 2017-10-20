#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"
#include "Driver/DS18B20.h"
#include "assert.h"

Software::OWI<BOARD::D7> owi;
DS18B20 sensor(owi);

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  // Set thermometer sensor alarm triggers (20..25 C) and resolution
  // (10 bits). Iterate all sensors and write configuration.
  uint8_t* rom = sensor.rom();
  int8_t last = owi.FIRST;
  do {
    last = owi.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == owi.ERROR) break;
    sensor.resolution(10);
    sensor.set_trigger(20, 25);
    ASSERT(sensor.write_scratchpad(false));
  } while (last != owi.LAST);
}

void loop()
{
  // Check if any thermometer sersors have exceeded thresholds.
  // Broadcast a convert request to all sensors: Print timestamp,
  // sensor identity (rom) and temperature for all that report an
  // alarm

  int8_t last = owi.FIRST;
  uint8_t* rom = sensor.rom();
  static uint16_t timestamp = 0;
  uint8_t id = 0;
  if (!sensor.convert_request(true)) return;
  delay(sensor.conversion_time());
  do {
    last = owi.alarm_search(rom, last);
    if (last == owi.ERROR) break;
    ASSERT(sensor.read_scratchpad(false));
    Serial.print(timestamp);
    Serial.print('.');
    Serial.print(id++);
    Serial.print(F(":rom="));
    for (size_t i = 0; i < owi.ROM_MAX; i++) {
      if (rom[i] < 0x10) Serial.print(0);
      Serial.print(rom[i], HEX);
    }
    Serial.print(F(",temperature="));
    Serial.println(sensor.temperature());
  } while (last != owi.LAST);
  timestamp += 1;
  delay(1000);
}
