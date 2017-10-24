/**
 * @file Driver/Arduino.h
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2017, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef OWI_DRIVER_ARDUINO_H
#define OWI_DRIVER_ARDUINO_H

#include "OWI.h"

// Configure: Slave device addressing; skip_rom, match_rom, or match_label
// Use skip_rom for single device on bus otherwise match_rom or match_label
#define MATCH() m_owi.skip_rom()
// #define MATCH() m_owi.match_rom(m_rom)
// #define MATCH() m_owi.match_label(m_label)

/**
 * One-Wire Interface (OWI) Remote Arduino Device Driver. Core
 * functions are implemented as one-wire communication. See
 * OWI/examples/Slave, Slave and Master sketches for examples.
 */
class Arduino : public OWI::Device {
public:
  /** Family code. */
  static const uint8_t FAMILY_CODE = 0x60;

  /**
   * Construct One-Wire Interface (OWI) Remote Arduino with given bus
   * and device address.
   * @param[in] owi bus manager.
   * @param[in] rom code (default NULL).
   */
  Arduino(OWI& owi, uint8_t* rom = NULL) :
    OWI::Device(owi, rom),
    m_label(0)
  {
  }

  /**
   * Get device label.
   * @return short address.
   */
  uint8_t label()
  {
    return (m_label);
  }

  /**
   * Set device label.
   * @param[in] label short address.
   */
  void label(uint8_t nr)
  {
    m_label = nr;
  }

  /**
   * Set given pin to given mode (OUTPUT, INPUT, INPUT_PULLUP).
   * @param[in] pin digital pin number.
   * @param[in] mode pin mode.
   * @return zero(0) or negative error code.
   */
  int pinMode(int pin, int mode)
  {
    if (!MATCH()) return (-1);
    m_owi.write(PIN_MODE);
    m_owi.write(pin, 6);
    m_owi.write(mode, 2);
    return (0);
  }

  /**
   * Read given pin and return current state or negative error code.
   * @param[in] pin digital pin number.
   * @return pin state (LOW, HIGH) or negative error code.
   */
  int digitalRead(int pin)
  {
    if (!MATCH()) return (-1);
    m_owi.write(DIGITAL_READ);
    m_owi.write(pin, 6);
    return (m_owi.read(1));
  }

  /**
   * Write given value to given pin. Return zero(0) if successful,
   * otherwise negative error code.
   * @param[in] pin digital pin number.
   * @param[in] value to write pin.
   * @return zero(0) or negative error code.
   */
  int digitalWrite(int pin, int value)
  {
    if (!MATCH()) return (-1);
    m_owi.write(DIGITAL_WRITE);
    m_owi.write(pin, 6);
    m_owi.write(value != 0, 1);
    return (0);
  }

  /**
   * Read analog value from given pin. Return read value if successful,
   * otherwise negative error code.
   * @param[in] pin analog pin number.
   * @return value read or negative error code.
   */
  int analogRead(int pin)
  {
    if (!MATCH()) return (-1);
    analog_read_res_t res;
    m_owi.write(ANALOG_READ);
    m_owi.write(pin, 6);
    delayMicroseconds(200);
    if (!m_owi.read(&res, sizeof(res))) return (-1);
    return (res.value);
  }

  /**
   * Set given duty to given pulse width modulated (PWM) pin. Return
   * zero(0) if successful, otherwise negative error code.
   * @param[in] pin digital pin number.
   * @param[in] duty of pulse width.
   * @return zero(0) or negative error code.
   */
  int analogWrite(int pin, int duty)
  {
    if (!MATCH()) return (-1);
    m_owi.write(ANALOG_WRITE);
    m_owi.write(pin, 6);
    m_owi.write(duty);
    return (0);
  }

  /**
   * Return number of digital pins, if successful otherwise negative
   * error code.
   * @return pins or negative error code.
   */
  int num_digital_pins()
  {
    if (!MATCH()) return (-1);
    m_owi.write(DIGITAL_PINS);
    return (m_owi.read(6));
  }

  /**
   * Return number of analog inputs, if successful otherwise negative
   * error code.
   * @return pins or negative error code.
   */
  int num_analog_inputs()
  {
    if (!MATCH()) return (-1);
    m_owi.write(ANALOG_PINS);
    return (m_owi.read(6));
  }

  /**
   * Read rom identity code for device. Return zero(0) if successful,
   * otherwise negative error code.
   * @return zero(0) or negative error code.
   */
  int read_rom()
  {
    if (m_owi.read_rom(m_rom)) return (0);
    return (-1);
  }

  /**
   * Set rom label for current addressed device. Return zero(0) if
   * successful, otherwise negative error code.
   * @param[in] nr label number.
   * @return zero(0) or negative error code.
   */
  int label_rom(uint8_t nr)
  {
    m_owi.write(OWI::LABEL_ROM);
    m_owi.write(nr);
    m_label = nr;
    return (0);
  }

  /**
   * Print device rom idenity code to given output stream. Return
   * zero(0) if successful, otherwise negative error code.
   * @param[in] out output stream (default Serial).
   * @return zero(0) or negative error code.
   */
  int print_rom(Print& out = Serial)
  {
    out.print(F("family="));
    if (m_rom[0] < 0x10) out.print('0');
    out.print(m_rom[0], HEX);
    out.print(F(",rom="));
    for (size_t i = 1; i < OWI::ROM_MAX - 1; i++) {
      if (m_rom[i] < 0x10) out.print('0');
      out.print(m_rom[i], HEX);
    }
    out.print(F(",crc="));
    uint8_t crc = m_rom[sizeof(OWI::ROM_MAX) - 1];
    if (crc < 0x10) out.print('0');
    out.print(crc, HEX);
    return (0);
  }

  /** One-Wire Interface (OWI) Remote Arduino Device function codes. */
  enum {
    PIN_MODE = 0x11,		//!< Set pin mode: 6b pin, 2b mode
    DIGITAL_READ = 0x22,	//!< Read digital pin: 6b pin, 1b return
    DIGITAL_WRITE = 0x33,	//!< Write digital pin: 6b pin, 1b value
    ANALOG_READ = 0x44,		//!< Read analog pin: 6b pin, 16b+8b return
    ANALOG_WRITE = 0x55,	//!< Write analog pin: 6b pin, 8b duty
    SRAM_READ = 0x66,		//!< SRAM read
    SRAM_WRITE = 0x77,		//!< SRAM write
    EEPROM_READ = 0x88,		//!< EEPROM read
    EEPROM_WRITE = 0x99,	//!< EEPROM write
    DIGITAL_PINS = 0xaa,	//!< Get number of digital pins: 6b return
    ANALOG_PINS = 0xbb		//!< Get number of analog inputs: 6b return
  };

protected:
  /** Short address. */
  uint8_t m_label;

  /** Return value for ANALOG_READ. */
  struct analog_read_res_t {
    uint16_t value;		//!< Analog value read.
    uint8_t crc;		//!< Cyclic Redundancy Check-sum.
  } __attribute__((packed));
};

#undef MATCH

#endif
