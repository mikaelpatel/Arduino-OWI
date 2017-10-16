#include "GPIO.h"
#include "OWI.h"
#include "Slave/OWI.h"

// Slave device; extended commands
enum {
  PIN_MODE = 0x11,
  DIGITAL_READ = 0x22,
  DIGITAL_WRITE = 0x33,
  ANALOG_READ = 0x44,
  ANALOG_WRITE = 0x55
};

// ROM identity code for slave device
uint8_t ROM[OWI::ROM_MAX] = {
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
};

// Slave device
Slave::OWI<BOARD::D7> owi(ROM);

void setup()
{
  pinMode(13,OUTPUT);
}

void loop()
{
  // Wait for extended commands
  if (!owi.rom_command()) return;

  uint16_t value;
  uint8_t pin;
  uint8_t mode;
  uint8_t crc;
  switch (owi.read()) {
  case PIN_MODE:
    pin = owi.read();
    mode = owi.read();
    pinMode(pin, mode);
    break;
  case DIGITAL_READ:
    pin = owi.read();
    value = digitalRead(pin);
    owi.write(value,1);
    break;
  case DIGITAL_WRITE:
    pin = owi.read();
    value = owi.read(1);
    digitalWrite(pin,value);
    break;
  case ANALOG_READ:
    pin = owi.read();
    value = analogRead(pin);
    owi.alarm(value > 512);
    owi.write(&value, sizeof(value));
    break;
  }
}
