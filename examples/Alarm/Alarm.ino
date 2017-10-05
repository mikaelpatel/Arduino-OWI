#include "RTC.h"
#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"
#include "Driver/DS18B20.h"

#if defined(ARDUINO_attiny)
#include "Software/Serial.h"
Software::Serial<BOARD::D0> Serial;
Software::OWI<BOARD::D1> owi;
#else
Software::OWI<BOARD::D7> owi;
#endif

DS18B20 sensor(owi);
RTC rtc;

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  // Set sensor alarm triggers (20..25 C) and resolution (10 bits)
  // Iterate though all thermometers and configure.
  uint8_t* rom = sensor.rom();
  int8_t last = owi.FIRST;
  do {
    last = owi.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == owi.ERROR) break;
    sensor.resolution(10);
    sensor.set_trigger(20, 25);
    sensor.write_scratchpad();
  } while (last != owi.LAST);
}

void loop()
{
  // Check if any thermometer sersors have exceeded thresholds.
  // Broadcast a convert request to all thermometer sensors.
  // Print timestamp and sensor identity and temperature.

  int8_t last = owi.FIRST;
  uint8_t* rom = sensor.rom();
  bool triggered = false;

  if (!rtc.tick()) return;
  if (!sensor.convert_request(true)) return;
  do {
    last = owi.alarm_search(rom, last);
    if (last == owi.ERROR) break;
    sensor.read_scratchpad(false);
    if (!triggered) {
      char daytime[32];
      struct tm now;
      rtc.get_time(now);
      isotime_r(&now, daytime);
      Serial.print(daytime + 11);
      triggered = true;
    }
    Serial.print(F(", "));
    for (size_t i = 1; i < owi.ROM_MAX - 1; i++) {
      if (rom[i] < 0x10) Serial.print(0);
      Serial.print(rom[i], HEX);
    }
    Serial.print(F(", "));
    Serial.print(sensor.temperature());
  } while (last != owi.LAST);
  if (triggered) Serial.println();
}
