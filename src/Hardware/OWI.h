/**
 * @file Hardware/OWI.h
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2017, Mikael Patel
 *
 * This library is free hardware; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Hardware Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef HARDWARE_OWI_H
#define HARDWARE_OWI_H

#include "OWI.h"
#include "Driver/DS2482.h"

/**
 * One Wire Interface (OWI) Bus Manager class using DS2482,
 * Single-Channel 1-Wire Master, TWI to OWI Bridge Device.
 */
namespace Hardware {
class OWI : public ::OWI {
public:
  /**
   * Construct one wire bus manager for DS2482.
   * @param[in] twi bus manager.
   * @param[in] subaddr sub-address for device.
   */
  OWI(TWI& twi, uint8_t subaddr = 0) :
    m_bridge(twi, 0x18 | (subaddr & 0x03))
  {
  }

  /**
   * @override{OWI}
   * Reset the one wire bus and check that at least one device is
   * presence.
   * @return true(1) if successful otherwise false(0).
   */
  virtual bool reset()
  {
    return (m_bridge.one_wire_reset());
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
    uint8_t res = 0;
    if (bits == CHARBITS) {
      m_bridge.one_wire_read_byte(res);
    }
    else {
      uint8_t adjust = CHARBITS - bits;
      bool value = 0;
      while (bits--) {
	res >>= 1;
	m_bridge.one_wire_read_bit(value);
	res |= (value ? 0x80 : 0x00);
      }
      res >>= adjust;
    }
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
    if (bits == CHARBITS) {
      m_bridge.one_wire_write_byte(value);
    }
    else {
      while (bits--) {
	m_bridge.one_wire_write_bit(value & 0x01);
	value >>= 1;
      }
    }
  }

  /**
   * @override{OWI}
   * Search (rom and alarm) support function. Reads 2-bits and writes
   * given direction 1-bit value when discrepancy 0b00 read. Writes
   * one(1) when 0b01 read, zero(0) on 0b10. Reading 0b11 is an error
   * state.
   * @param[in,out] dir bit to write when discrepancy read.
   * @return 2-bits read and bit written.
   */
  virtual int8_t triplet(uint8_t& dir)
  {
    return (m_bridge.one_wire_triplet(dir));
  }

  /**
   * Global reset of device state machine logic. Returns true if
   * successful otherwise false.
   * @return bool.
   */
  bool device_reset()
  {
    return (m_bridge.device_reset());
  }

  /**
   * Configure one wire bus master with given parameters. Returns true
   * if successful otherwise false.
   * @param[in] apu active pull-up (default true).
   * @param[in] spu strong pull-up (default false).
   * @param[in] iws one wire speed (default false).
   * @return bool.
   */
  bool device_configuration(bool apu = true, bool spu = false, bool iws = false)
  {
    return (m_bridge.write_configuration(apu, spu, iws));
  }

  /**
   * Select given channel (DS2482-800). Return true if successful
   * otherwise false.
   * @param[in] chan channel number (0..7).
   * @return bool.
   */
  bool channel_select(uint8_t chan)
  {
    return (m_bridge.channel_select(chan));
  }

protected:
  DS2482 m_bridge;
};
};
#endif
