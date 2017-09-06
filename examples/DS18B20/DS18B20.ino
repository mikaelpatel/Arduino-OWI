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

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  uint8_t rom[owi.ROM_MAX] = { 0 };
  int8_t last = owi.FIRST;
  int i = 0;

  // Broadcast a convert request to all thermometer sensors
  sensor.convert_request(true);

  // Print list of sensors and temperature
  do {
    last = owi.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == owi.ERROR) break;
    sensor.read_scratchpad(false);
    Serial.print(i++);
    Serial.print(F(":ROM:"));
    for (size_t i = 0; i < sizeof(rom); i++) {
      Serial.print(' ');
      Serial.print(rom[i], HEX);
    }
    Serial.print(F(": "));
    Serial.print(sensor.temperature());
    Serial.println(F(" C"));
  } while (last != owi.LAST);
  Serial.println();

  delay(5000);
}
