/**
 * @file OWI.h
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

#ifndef OWI_H
#define OWI_H

#include "GPIO.h"

#ifndef CHARBITS
#define CHARBITS 8
#endif

/**
 * One Wire Interface (OWI) template class using GPIO.
 * @param[in] PIN board pin for 1-wire bus.
 */
template<BOARD::pin_t PIN>
class OWI {
public:
  /**
   * Standard ROM Commands.
   */
  enum {
    SEARCH_ROM = 0xF0,		//!< Initiate device search.
    READ_ROM = 0x33,		//!< Read device family code and serial number.
    MATCH_ROM = 0x55,		//!< Select device with 64-bit rom code.
    SKIP_ROM = 0xCC,		//!< Broadcast or single device.
    ALARM_SEARCH = 0xEC		//!< Initiate device alarm search.
  } __attribute__((packed));

  /** ROM size in bytes. */
  static const size_t ROM_MAX = 8;

  /** ROM size in bits. */
  static const size_t ROMBITS = ROM_MAX * CHARBITS;

  /**
   * Construct one wire bus connected to the given template pin
   * parameter.
   */
  OWI() :
    m_crc(0)
  {
    m_pin.input();
    m_pin.low();
  }

  /**
   * Reset the one wire bus and check that at least one device is
   * presence.
   * @return true(1) if successful otherwise false(0).
   */
  bool reset()
  {
    uint8_t retry = RESET_RETRY_MAX;
    bool res;
    do {
      m_pin.output();
      delayMicroseconds(490);
      noInterrupts();
      m_pin.input();
      delayMicroseconds(70);
      res = m_pin;
      interrupts();
      delayMicroseconds(410);
    } while (retry-- && res);
    return (res == 0);
  }

  /**
   * Read the given number of bits from the one wire bus. Default
   * number of bits is 8. Calculate partial check-sum.
   * @param[in] bits to be read.
   * @return value read.
   */
  uint8_t read(uint8_t bits = CHARBITS)
  {
    uint8_t adjust = CHARBITS - bits;
    uint8_t res = 0;
    uint8_t mix = 0;
    while (bits--) {
      noInterrupts();
      m_pin.output();
      delayMicroseconds(6);
      m_pin.input();
      delayMicroseconds(9);
      res >>= 1;
      if (m_pin) {
	res |= 0x80;
	mix = (m_crc ^ 1);
      }
      else {
	mix = (m_crc ^ 0);
      }
      m_crc >>= 1;
      if (mix & 1) m_crc ^= 0x8C;
      interrupts();
      delayMicroseconds(55);
    }
    res >>= adjust;
    return (res);
  }

  /**
   * Read given number of bytes from one wire bus (device) to given
   * buffer. Return true(1) if correctly read otherwise false(0).
   * @param[in] buf buffer pointer.
   * @param[in] count number of bytes to read.
   * @return bool.
   */
  bool read(void* buf, size_t count)
  {
    uint8_t* bp = (uint8_t*) buf;
    m_crc = 0;
    while (count--) *bp++ = read();
    return (m_crc == 0);
  }

  /**
   * Write the given value to the one wire bus. The bits are written
   * from LSB to MSB.
   * @param[in] value to write.
   * @param[in] bits to be written.
   */
  void write(uint8_t value, uint8_t bits = CHARBITS)
  {
    while (bits--) {
      noInterrupts();
      m_pin.output();
      if (value & 0x01) {
	delayMicroseconds(6);
	m_pin.input();
	delayMicroseconds(64);
      }
      else {
	delayMicroseconds(60);
	m_pin.input();
	delayMicroseconds(10);
      }
      interrupts();
      value >>= 1;
    }
  }

  /**
   * Write the given value and given number of bytes from buffer to
   * the one wire bus (device).
   * @param[in] value to write.
   * @param[in] buf buffer pointer.
   * @param[in] count number of bytes to write.
   */
  void write(uint8_t value, const void* buf, size_t count)
  {
    write(value);
    const uint8_t* bp = (const uint8_t*) buf;
    while (count--) write(*bp++);
  }

  /** Search position. */
  enum {
    FIRST = -1,			//!< Start position of search.
    ERROR = -1,			//!< Error during search.
    LAST = ROMBITS		//!< Last position, search completed.
  } __attribute__((packed));

  /**
   * Search device rom given the last position of discrepancy.
   * @param[in] family code.
   * @param[in] code device identity.
   * @param[in] last position of discrepancy (default FIRST).
   * @return position of difference or negative error code.
   */
  int8_t search_rom(uint8_t family, uint8_t* code, int8_t last = FIRST)
  {
    do {
      if (!reset()) return (ERROR);
      write(SEARCH_ROM);
      last = search(code, last);
    } while ((last != LAST) && (family != 0) && (code[0] != family));
    return (last);
  }

  /**
   * Read device rom. This can only be used when there is only
   * one device on the bus.
   * @param[in] code device identity.
   * @return true(1) if successful otherwise false(0).
   */
  bool read_rom(uint8_t* code)
  {
    if (!reset()) return (false);
    write(READ_ROM);
    return (read(code, ROM_MAX));
  }

  /**
   * Match device rom. Address the device with the rom code. Device
   * specific function command should follow. May be used to verify
   * rom code.
   * @param[in] code device identity.
   * @return true(1) if successful otherwise false(0).
   */
  bool match_rom(uint8_t* code)
  {
    if (!reset()) return (false);
    write(MATCH_ROM, code, ROM_MAX);
    return (true);
  }

  /**
   * Skip device rom for boardcast or single device access.
   * Device specific function command should follow.
   * @return true(1) if successful otherwise false(0).
   */
  bool skip_rom()
  {
    if (!reset()) return (false);
    write(SKIP_ROM);
    return (true);
  }

  /**
   * Search alarming device given the last position of discrepancy.
   * @param[in] code device identity.
   * @param[in] last position of discrepancy (default FIRST).
   * @return position of difference or negative error code.
   */
  int8_t alarm_search(uint8_t* code, int8_t last = FIRST)
  {
    if (!reset()) return (ERROR);
    write(ALARM_SEARCH);
    return (search(code, last));
  }

protected:
  /** Maximum number of reset retries. */
  static const uint8_t RESET_RETRY_MAX = 4;

  /** 1-Wire bus pin. */
  GPIO<PIN> m_pin;

  /** Intermediate CRC sum. */
  uint8_t m_crc;

  /**
   * Search device rom given the last position of discrepancy.
   * @param[in] code device identity.
   * @param[in] last position of discrepancy.
   * @return position of difference or negative error code.
   */
  int8_t search(uint8_t* code, int8_t last = FIRST)
  {
    uint8_t pos = 0;
    int8_t next = LAST;
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t data = 0;
      for (uint8_t j = 0; j < 8; j++) {
	data >>= 1;
	switch (read(2)) {
	case 0b00: // Discrepency between device roms
	  if (pos == last) {
	    write(1, 1);
	    data |= 0x80;
	    last = FIRST;
	  }
	  else if (pos > last) {
	    write(0, 1);
	    next = pos;
	  }
	  else if (code[i] & (1 << j)) {
	    write(1, 1);
	    data |= 0x80;
	  }
	  else {
	    write(0, 1);
	    next = pos;
	  }
	  break;
	case 0b01: // Only one's at this position
	  write(1, 1);
	  data |= 0x80;
	  break;
	case 0b10: // Only zero's at this position
	  write(0, 1);
	  break;
	case 0b11: // No device detected
	  return (ERROR);
	}
	pos += 1;
      }
      code[i] = data;
    }
    return (next);
  }
};
#endif
