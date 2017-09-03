/**
 * @file DS18B20.h
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

#ifndef DS18B20_H
#define DS18B20_H

#include "OWI.h"

/**
 * Driver for the DS18B20 Programmable Resolution 1-Write Digital
 * Thermometer.
 *
 * @section Circuit
 * @code
 *                           DS18B20
 * (VCC)--[4K7]--+       +------------+
 * (GND)---------)-----1-|GND         |\
 * (Dn)----------+-----2-|DQ          | |
 * (VCC)---------------3-|VDD         |/
 *                       +------------+
 *
 * @endcode
 *
 * @section References
 * 1. Maxim Integrated product description (REV: 042208)
 */
template<BOARD::pin_t PIN>
class DS18B20 : public OWI<PIN> {
public:
  /** Device family code. */
  static const uint8_t FAMILY_CODE = 0x28;

  /**
   * Construct a DS18B20 device connected to the given 1-Wire bus.
   */
  DS18B20() :
    OWI<PIN>(),
    m_start(0L),
    m_converting(false)
  {}

  /**
   * Set conversion resolution from 9..12 bits. Use write_scratchpad()
   * and copy_scratchpad() to update device.
   * @param[in] bits resolution.
   */
  void resolution(uint8_t bits)
  {
    if (bits < 9) bits = 9; else if (bits > 12) bits = 12;
    m_scratchpad.configuration = (((bits - 9) << 5) | 0x1f);
  }

  /**
   * Set alarm trigger values; low and high threshold values.
   * Use write_scratchpad() and copy_scratchpad() to update device.
   * @param[in] low threshold.
   * @param[in] high threshold.
   */
  void set_trigger(int8_t low, int8_t high)
  {
    m_scratchpad.low_trigger = low;
    m_scratchpad.high_trigger = high;
  }

  /**
   * Get the latest temperature reading from the local memory scratchpad.
   * Call convert_request() and read_scratchpad() before accessing the
   * scratchpad. Returns at highest resolution a fixed point<12,4>
   * point number. For 11-bit resolution, bit 0 is undefined, 10-bits
   * bit 1 and 0, and so on (LSB).
   * @return temperature
   */
  float temperature() const
  {
    return (m_scratchpad.temperature * 0.0625);
  }

  /**
   * Get conversion resolution. Use connect(), or read_scratchpad() to
   * read values from device before calling this method.
   * @return number of bits.
   */
  uint8_t resolution() const
  {
    return (9 + (m_scratchpad.configuration >> 5));
  }

  /**
   * Get alarm trigger values; low and high threshold values.
   * Use connect(), or read_scratchpad() to read values from device.
   * @param[out] low threshold.
   * @param[out] high threshold.
   */
  void get_trigger(int8_t& low, int8_t& high) const
  {
    low = m_scratchpad.low_trigger;
    high = m_scratchpad.high_trigger;
  }

  /**
   * Initiate temperature conversion. Pass NULL for rom code to
   * broadcast to all sensors.
   * @param[in] code device identity (default NULL).
   * @return true(1) if successful otherwise false(0).
   */
  bool convert_request(uint8_t* code = NULL)
  {
    if (code != NULL) {
      if (!match_rom(code)) return (false);
    }
    else {
      if (!skip_rom()) return (false);
    }
    write(CONVERT_T, CHARBITS);
    m_start = millis();
    m_converting = true;
    return (true);
  }

  /**
   * Read the contents of the scratchpad to local memory. An internal
   * delay will occur if a convert_request() is pending. The delay is
   * at most max conversion time (750 ms). Pass NULL for rom code to
   * current sensor in search.
   * @param[in] code device identity (default NULL).
   * @return true(1) if successful otherwise false(0).
   */
  bool read_scratchpad(uint8_t* code = NULL)
  {
    if (m_converting) {
      int32_t ms = millis() - m_start;
      uint16_t conv_time = (MAX_CONVERSION_TIME >> (12 - resolution()));
      if (ms < conv_time) {
	ms = conv_time - ms;
	delay(ms);
      }
      m_converting = false;
    }
    if (code && !match_rom(code)) return (false);
    write(READ_SCRATCHPAD);
    return (read(&m_scratchpad, sizeof(m_scratchpad)));
  }

  /**
   * Write the contents of the scratchpad triggers and configuration
   * (3 bytes) to device.
   * @param[in] code device identity.
   * @return true(1) if successful otherwise false(0).
   */
  bool write_scratchpad(uint8_t* code)
  {
    if (!match_rom(code)) return (false);
    write(WRITE_SCRATCHPAD, &m_scratchpad.high_trigger, CONFIG_MAX);
    return (true);
  }

  /**
   * Copy device scratchpad triggers and configuration data to device
   * EEPROM.
   * @param[in] code device identity.
   * @return true(1) if successful otherwise false(0).
   */
  bool copy_scratchpad(uint8_t* code)
  {
    if (!match_rom(code)) return (false);
    write(COPY_SCRATCHPAD);
    return (true);
  }

  /**
   * Recall the alarm triggers and configuration from device EEPROM.
   * @param[in] code device identity.
   * @return true(1) if successful otherwise false(0).
   */
  bool recall(uint8_t* code)
  {
    if (!match_rom(code)) return (false);
    write(RECALL_E);
    return (true);
  }

  using OWI<PIN>::read;
  using OWI<PIN>::write;
  using OWI<PIN>::skip_rom;
  using OWI<PIN>::match_rom;

protected:
  /**
   * DS18B20 Function Commands (Table 3, pp. 12).
   */
  enum {
    CONVERT_T = 0x44,		//!< Initiate temperature conversion.
    READ_SCRATCHPAD = 0xBE,	//!< Read scratchpad including crc byte.
    WRITE_SCRATCHPAD = 0x4E,	//!< Write data to scratchpad.
    COPY_SCRATCHPAD = 0x48,	//!< Copy configuration register to EEPROM.
    RECALL_E = 0xB8,		//!< Recall configuration data from EEPROM.
    READ_POWER_SUPPLY = 0xB4	//!< Signal power supply mode.
  } __attribute__((packed));

  /**
   * DS18B20 Memory Map (Figure 7, pp. 7).
   */
  struct scratchpad_t {
    int16_t temperature;	//!< Temperature reading (9-12 bits).
    int8_t high_trigger;	//!< High temperature trigger.
    int8_t low_trigger;		//!< Low temperature trigger.
    uint8_t configuration;	//!< Configuration; resolution, alarm.
    uint8_t reserved[3];	//!< Reserved.
    uint8_t crc;		//!< Check sum.
  };
  scratchpad_t m_scratchpad;

  /** Size of configuration; high/low trigger and configuration byte. */
  static const uint8_t CONFIG_MAX = 3;

  /** Watchdog millis on convert_request(). */
  uint32_t m_start;

  /** Convert request pending. */
  uint8_t m_converting;

  /** Max conversion time for 12-bit conversion in milli-seconds. */
  static const uint16_t MAX_CONVERSION_TIME = 750;
};
#endif
