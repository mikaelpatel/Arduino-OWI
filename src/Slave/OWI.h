/**
 * @file Slave/OWI.h
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

#ifndef SLAVE_OWI_H
#define SLAVE_OWI_H

#include "GPIO.h"

#ifndef CHARBITS
#define CHARBITS 8
#endif

/**
 * One Wire Interface (OWI) Slave Device template class using GPIO.
 * Allows emulation of One Wire devices, and basic communication
 * between boards. Supports the standard ROM commands.
 * @param[in] PIN board pin for 1-wire bus.
 */
namespace Slave {
template<BOARD::pin_t PIN>
class OWI {
public:
  /** One Wire device identity ROM size in bytes. */
  static const size_t ROM_MAX = 8;

  /** One Wire device identity ROM size in bits. */
  static const size_t ROMBITS = ROM_MAX * CHARBITS;

  /**
   * Construct one wire bus slave device connected to the given
   * template pin parameter, and rom identity code. Cyclic redundancy
   * check sum is generated for given rom identity code.
   * @param[in] rom identity code.
   */
  OWI(uint8_t* rom) :
    m_timestamp(0),
    m_rom(rom),
    m_alarm(false)
  {
    uint8_t crc = 0;
    for (size_t i = 0; i < ROM_MAX - 1; i++)
      crc = crc_update(crc, m_rom[i]);
    m_rom[ROM_MAX - 1] = crc;
    m_pin.open_drain();
  }

  /**
   * Get alarm setting.
   * @return alarm setting.
   */
  bool alarm()
  {
    return (m_alarm);
  }

  /**
   * Set alarm to given value.
   * @param[in] value alarm setting.
   */
  void alarm(bool value)
  {
    m_alarm = value;
  }

  /**
   * Check for reset signal. Return true(1) if reset was detected and
   * presence was signaled, otherwise false(0).
   * @return true(1) on reset and presence, otherwise false(0).
   */
  bool reset()
  {
    // Check reset start
    if (m_timestamp == 0) {
      if (!m_pin) m_timestamp = micros();
      return (false);
    }

    // Check reset pulse stop
    if (!m_pin) return (false);

    // Check reset pulse width
    if (micros() - m_timestamp < 400) {
      m_timestamp = 0;
      return (false);
    }

    // Generate presence signal
    m_pin.output();
    delayMicroseconds(100);
    m_pin.input();

    // Wait for possible presence signals from other devices
    while (!m_pin);
    m_timestamp = 0;

    return (true);
  }

  /**
   * Read bits from one wire bus master. Default number of bits is 8.
   * @param[in] bits to be read.
   * @return value read.
   */
  uint8_t read(uint8_t bits = CHARBITS)
  {
    uint8_t adjust = CHARBITS - bits;
    uint8_t res = 0;
    while (bits--) {
      noInterrupts();
      // Wait for bit start
      while (m_pin);
      // Delay to sample bit value
      delayMicroseconds(15);
      res >>= 1;
      res |= (m_pin ? 0x80 : 0x00);
      interrupts();
      // Wait for bit end
      m_timestamp = micros();
      for (int i = 200; !m_pin; i--) if (i == 0) return (0);
    }
    m_timestamp = 0;
    res >>= adjust;
    return (res);
  }

  /**
   * Read given number of bytes from one wire bus master to given
   * buffer. Calculates 1-wire crc and return result of validation.
   * @param[in] buf buffer pointer.
   * @param[in] count number of bytes to read.
   * @return true(1) if check sum is correct otherwise false(0).
   */
  bool read(void* buf, size_t count)
  {
    uint8_t* bp = (uint8_t*) buf;
    uint8_t crc = 0;

    // Write bytes and calculate crc
    while (count--) {
      uint8_t value = read();
      *bp++ = value;
      crc = crc_update(crc, value);
    }

    // Return crc validation
    return (crc == 0);
  }

  /**
   * Write bits to one wire bus master. The bits are written from LSB
   * to MSB. Default number of bits is 8.
   * @param[in] value to write.
   * @param[in] bits to be written.
   */
  void write(uint8_t value, uint8_t bits = CHARBITS)
  {
    while (bits--) {
      noInterrupts();
      // Wait for bit start
      while (m_pin);
      // Streck low if bit is zero
      if ((value & 0x01) == 0) {
	m_pin.output();
	delayMicroseconds(20);
	m_pin.input();
      }
      interrupts();
      value >>= 1;
      // Wait for bit end, other devices might be responding
      m_timestamp = micros();
      for (int i = 200; !m_pin; i--) if (i == 0) return;
    }
    m_timestamp = 0;
  }

  /**
   * Write bytes to one wire bus master. Calculates and writes 1-wire
   * cyclic redundancy check-sum last.
   * @param[in] buf buffer to write.
   * @param[in] count number of bytes to write.
   */
  void write(const void* buf, size_t count)
  {
    const uint8_t* bp = (const uint8_t*) buf;
    uint8_t crc = 0;

    // Write bytes and calculate crc
    while (count--) {
      uint8_t value = *bp++;
      write(value);
      crc = crc_update(crc, value);
    }
    write(crc);
  }

  /**
   * Standard 1-Wire ROM Commands.
   */
  enum {
    SEARCH_ROM = 0xF0,		//!< Initiate device search.
    READ_ROM = 0x33,		//!< Read device family code and serial number.
    MATCH_ROM = 0x55,		//!< Select device with 64-bit rom code.
    SKIP_ROM = 0xCC,		//!< Broadcast or single device.
    ALARM_SEARCH = 0xEC		//!< Initiate device alarm search.
  } __attribute__((packed));

  /**
   * Check for reset and standard rom commands. Returns true(1) if
   * the device was selected and extended command will follow,
   * otherwise false(0).
   * @return true(1) on extended command will follow, otherwise false(0).
   */
  bool rom_command()
  {
    // Wait for reset
    if (!reset()) return (false);

    // Standard ROM commands
    switch (read()) {
    case OWI::READ_ROM:
      // Write ROM to master
      write(m_rom, ROM_MAX - 1);
      return (false);
    case OWI::MATCH_ROM:
      // Match ROM from master
      for (size_t i = 0; i < ROM_MAX; i++)
	if (m_rom[i] != read())
	  return (false);
      // Skip ROM, extended command will follow
    case OWI::SKIP_ROM:
      return (true);
    case OWI::ALARM_SEARCH:
      // Ignore if alarm is not set
      if (!m_alarm)
	return (false);
    case OWI::SEARCH_ROM:
      // Write ROM bit and check branching
      for (size_t i = 0; i < ROM_MAX; i++) {
	uint8_t value = m_rom[i];
	uint8_t mask = 0x01;
	do {
	  uint8_t bit = (mask & value) != 0;
	  write(bit, 1);
	  write(!bit, 1);
	  bool dir = read(1);
	  if (dir != bit) return (false);
	  mask <<= 1;
	} while (mask);
      }
      return (true);
    default:
      // Ignore all other commands
      return (false);
    };
  }

  /**
   * Optimized Dallas/Maxim iButton 8-bit Cyclic Redundancy Check
   * calculation. Polynomial: x^8 + x^5 + x^4 + 1 (0x8C).
   * See http://www.maxim-ic.com/appnotes.cfm/appnote_number/27
   */
  static inline uint8_t crc_update(uint8_t crc, uint8_t data)
    __attribute__((always_inline))
  {
    crc = crc ^ data;
    uint8_t i = 8;
    do {
      if (crc & 0x01)
	crc = (crc >> 1) ^ 0x8C;
      else
	crc >>= 1;
    } while (--i);
    return (crc);
  }

protected:
  /** 1-Wire bus pin. */
  GPIO<PIN> m_pin;

  /** Reset detect timestamp. */
  uint32_t m_timestamp;

  /** ROM identity code. */
  uint8_t* m_rom;

  /** Alarm setting. */
  bool m_alarm;
};
};
#endif
