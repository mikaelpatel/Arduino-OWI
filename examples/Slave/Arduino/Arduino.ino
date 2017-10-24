#include "GPIO.h"
#include "OWI.h"
#include "Slave/OWI.h"
#include "Driver/Arduino.h"

/** One-Wire Slave Device Bus Manager. */
Slave::OWI<BOARD::D7> owi(Arduino::FAMILY_CODE);

/**
 * Setup One-Wire Remote Arduino Slave Device. Set alarm
 * state to allow alarm search before reading analog input.
 */
void setup()
{
  owi.alarm(true);
}

/**
 * Service incoming one-wire rom and remote arduino functions.
 */
void loop()
{
  // Additional sketch code could be placed here before
  // polling one-wire bus for commands
  if (!owi.rom_command()) return;

  const size_t BUF_MAX = 32;
  uint8_t buf[BUF_MAX];
  uint16_t value;
  uint8_t pin;
  uint8_t mode;
  uint8_t* bp;
  uint8_t count;
  uint8_t res;
  uint8_t duty;

  // Read and dispatch remote arduino commands
  switch (owi.read_command()) {
  case Arduino::PIN_MODE:
    pin = owi.read(6);
    mode = owi.read(2);
    pinMode(pin, mode);
    break;
  case Arduino::DIGITAL_READ:
    pin = owi.read(6);
    value = digitalRead(pin);
    owi.write(value, 1);
    break;
  case Arduino::DIGITAL_WRITE:
    pin = owi.read(6);
    value = owi.read(1);
    digitalWrite(pin, value);
    break;
  case Arduino::ANALOG_READ:
    pin = owi.read(6);
    value = analogRead(pin);
    owi.alarm(value > 512);
    owi.write(&value, sizeof(value));
    break;
  case Arduino::ANALOG_WRITE:
    pin = owi.read(6);
    duty = owi.read();
    analogWrite(pin, duty);
    break;
  case Arduino::SRAM_READ:
    owi.read(&bp, sizeof(bp));
    count = owi.read();
    owi.write(bp, count);
    break;
  case Arduino::SRAM_WRITE:
    owi.read(&bp, sizeof(bp));
    count = owi.read();
    owi.read(bp, count + 1);
    break;
  case Arduino::EEPROM_READ:
    owi.read(&bp, sizeof(bp));
    count = owi.read();
    owi.crc(0);
    while (count--) owi.write(eeprom_read_byte(bp++));
    owi.write(owi.crc());
    break;
  case Arduino::EEPROM_WRITE:
    owi.read(&bp, sizeof(bp));
    count = owi.read();
    res = 0b00;
    if (count > BUF_MAX) {
      while (count--) owi.read();
      owi.read();
    }
    else if (owi.read(buf, count)) {
      eeprom_write_block(buf, bp, count);
      res = 0b10;
    }
    owi.write(res, 2);
    break;
  case Arduino::DIGITAL_PINS:
    owi.write(NUM_DIGITAL_PINS, 6);
    break;
  case Arduino::ANALOG_PINS:
    owi.write(NUM_ANALOG_INPUTS, 6);
    break;
  }
}
