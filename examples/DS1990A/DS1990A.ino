#include "OWI.h"
#include "GPIO.h"
#include "Software/OWI.h"

#if defined(ARDUINO_attiny)
GPIO<BOARD::D0> led;
Software::OWI<BOARD::D1> owi;
#else
GPIO<BOARD::D13> led;
Software::OWI<BOARD::D7> owi;
#endif

// Table with valid keys (64 bit 1-Wire rom code, 8 bytes per entry)
const uint8_t KEY[] PROGMEM = {
  0x01, 0x23, 0x81, 0xa3, 0x09, 0x00, 0x00, 0x7b,
  0x01, 0x29, 0x01, 0x27, 0x09, 0x00, 0x00, 0xa8,
  0x01, 0x26, 0xd9, 0x3e, 0x09, 0x00, 0x00, 0x47
};

void setup()
{
  led.output();
  led = LOW;
}

void loop()
{
  // Attempt to read key every seconds
  uint8_t rom[owi.ROM_MAX] = { 0 };
  delay(1000);
  if (!owi.read_rom(rom)) return;

  // Check if it is an authorized key. One long blink if found
  for (uint8_t i = 0; i < sizeof(KEY); i += OWI::ROM_MAX) {
    if (!memcmp_P(rom, &KEY[i], owi.ROM_MAX)) {
      led = HIGH;
      delay(1000);
      led = LOW;
      return;
    }
  }

  // Three short blinks if not found
  for (uint8_t i = 0; i < 3; i++) {
    delay(250);
    led = HIGH;
    delay(250);
    led = LOW;
  }
}
