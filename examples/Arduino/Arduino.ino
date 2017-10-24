#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"
#include "Driver/Arduino.h"
#include "assert.h"
#include "benchmark.h"

// One-Wire Interface Remove Arduino (Master)
Software::OWI<BOARD::D7> owi;
Arduino arduino(owi);

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  uint8_t rom[owi.ROM_MAX] = { 0 };
  int8_t last = owi.FIRST;
  int id = 0;
  int pins;

  // Search for device and set short address (label)
  last = owi.search_rom(arduino.FAMILY, rom, last);
  ASSERT(last != owi.ERROR);
  arduino.rom(rom);
  arduino.label_rom(id);
  Serial.print(id);
  Serial.print(':');
  arduino.print_rom();
  Serial.println();

  // Get number of digital pins and read pin state
  ASSERT((pins = arduino.num_digital_pins()) > 0);
  Serial.print(F("arduino.num_digital_pins="));
  Serial.println(pins);
  Serial.println(F("arduino.digitalRead:"));
  for (int i = 0; i < pins; i++) {
    Serial.print('D');
    Serial.print(i);
    Serial.print(':');
    Serial.println(arduino.digitalRead(i));
  }

  // Get number of analog inputs and read analog values
  ASSERT((pins = arduino.num_analog_inputs()) > 0);
  Serial.print(F("arduino.num_analog_inputs="));
  Serial.println(pins);
  Serial.println(F("arduino.analogRead:"));
  for (int i = 0; i < pins; i++) {
    Serial.print('A');
    Serial.print(i);
    Serial.print(':');
    Serial.println(arduino.analogRead(i));
  }

  // Set builtin led pin to output
  ASSERT(arduino.pinMode(13, OUTPUT) == 0);

  // Set pulse width modulated pin(3) to 50% duty(127)
  ASSERT(arduino.analogWrite(3, 127) == 0);
}

void loop()
{
  // Classical blink sketch loop; measure micro-seconds to perform
  // one-wire interface remote arduino functions
  MEASURE(arduino.digitalWrite(13, HIGH));
  delay(1000);
  MEASURE(arduino.digitalWrite(13, LOW));
  delay(1000);
}
