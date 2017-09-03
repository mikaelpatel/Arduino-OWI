/**
 * @file Software/OWI.h
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

#ifndef SOFTWARE_OWI_H
#define SOFTWARE_OWI_H

#include "OWI.h"
#include "GPIO.h"

/**
 * One Wire Interface (OWI) template class using GPIO.
 * @param[in] PIN board pin for 1-wire bus.
 */
namespace Software {
template<BOARD::pin_t PIN>
  class OWI : public ::OWI {
public:
  /**
   * Construct one wire bus connected to the given template pin
   * parameter.
   */
  OWI()
  {
    m_pin.input();
    m_pin.low();
  }

  /**
   * @override{OWI}
   * Reset the one wire bus and check that at least one device is
   * presence.
   * @return true(1) if successful otherwise false(0).
   */
  virtual bool reset()
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
   * @override{OWI}
   * Read the given number of bits from the one wire bus. Default
   * number of bits is 8. Calculate partial check-sum.
   * @param[in] bits to be read.
   * @return value read.
   */
  virtual uint8_t read(uint8_t bits = CHARBITS)
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
   * @override{OWI}
   * Write the given value to the one wire bus. The bits are written
   * from LSB to MSB.
   * @param[in] value to write.
   * @param[in] bits to be written.
   */
  virtual void write(uint8_t value, uint8_t bits = CHARBITS)
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

protected:
  /** 1-Wire bus pin. */
  GPIO<PIN> m_pin;
};
};
#endif
