#include "GPIO.h"
#include "OWI.h"
#include "DS18B20.h"

DS18B20<BOARD::D7> sensor;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  uint8_t rom[sensor.ROM_MAX] = { 0 };
  int8_t last = sensor.FIRST;
  int i = 0;

  // Broadcast a convert request to all thermometer sensors
  sensor.convert_request();

  // Print list of sensors and temperature
  do {
    last = sensor.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == sensor.ERROR) break;
    sensor.read_scratchpad();
    Serial.print(i++);
    Serial.print(F(":ROM:"));
    for (size_t i = 0; i < sizeof(rom); i++) {
      Serial.print(' ');
      Serial.print(rom[i], HEX);
    }
    Serial.print(F(": "));
    Serial.print(sensor.temperature());
    Serial.println(F(" C"));
  } while (last != sensor.LAST);
  Serial.println();

  delay(5000);
}
