/**
 * @file Driver/DS18B20.h
 * @version 1.1
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
 * @endcode
 *
 * @section References
 * 1. Maxim Integrated product description (REV: 042208)
 */
class DS18B20 : public OWI::Device {
public:
  /** Device family code. */
  static const uint8_t FAMILY_CODE = 0x28;

  /** Max conversion time for 12-bit conversion in milli-seconds. */
  static const uint16_t MAX_CONVERSION_TIME = 750;

  /**
   * Construct a DS18B20 device connected to the given 1-Wire bus.
   * Initiate with default resolution (12-bits) and triggers (70, 75),
   * as hardware reset for device.
   * @param[in] owi bus manager.
   * @param[in] rom code (default NULL).
   */
  DS18B20(OWI& owi, uint8_t* rom = NULL) :
    OWI::Device(owi, rom),
    m_start(0),
    m_converting(false)
  {
    resolution(12);
    set_trigger(70, 75);
  }

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
   * Get the latest temperature reading from the scratchpad copy.
   * Call convert_request(), convert_await() and read_scratchpad()
   * before accessing the scratchpad.
   * @return temperature
   */
  float temperature() const
  {
    return (m_scratchpad.temperature * 0.0625);
  }

  /**
   * Get conversion resolution.
   * @return number of bits.
   */
  uint8_t resolution() const
  {
    return (9 + (m_scratchpad.configuration >> 5));
  }

  /**
   * Get alarm trigger values; low and high threshold values.
   * @param[out] low threshold.
   * @param[out] high threshold.
   */
  void get_trigger(int8_t& low, int8_t& high) const
  {
    low = m_scratchpad.low_trigger;
    high = m_scratchpad.high_trigger;
  }

  /**
   * Initiate temperature conversion. Call with broadcast parameter
   * true(1) to issue skip_rom() and issue to command to all devices.
   * @param[in] broadcast flag (default false).
   * @return true(1) if successful otherwise false(0).
   */
  bool convert_request(bool broadcast = false)
  {
    if (broadcast) {
      if (!m_owi.skip_rom()) return (false);
    }
    else {
      if (!m_owi.match_rom(m_rom)) return (false);
    }
    m_owi.write(CONVERT_T);
    m_start = millis();
    m_converting = true;
    return (true);
  }

  /**
   * Return remaining conversion time in milliseconds.
   * @return milliseconds remaining.
   */
  uint16_t conversion_time()
  {
    if (!m_converting) return (0);
    m_converting = false;
    uint16_t ms = millis() - m_start;
    uint16_t conv_ms = (MAX_CONVERSION_TIME >> (12 - resolution()));
    if (conv_ms > ms) return (conv_ms - ms);
    return (0);
  }

  /**
   * Check if the temperature conversion is completed.
   * @return true(1) if ready otherwise false(0).
   */
  bool convert_ready()
  {
    if (!m_converting) return (true);
    bool res = m_owi.read(1);
    if (res) m_converting = false;
    return (res);
  }

  /**
   * Delay until the temperature conversion is completed
   * by polling the sensors.
   * @return true(1) if successful otherwise false(0).
   */
  bool convert_await()
  {
    if (!m_converting) return (false);
    while (!convert_ready()) delay(1);
    return (true);
  }

  /**
   * Read the contents of the scratchpad to local memory. Call
   * convert_ready(), convert_await() or delay with amount from
   * conversion_time() before reading. Call with match parameter false
   * if used with search_rom().
   * @param[in] match rom code (default true).
   * @return true(1) if successful otherwise false(0).
   */
  bool read_scratchpad(bool match = true)
  {
    if (match && !m_owi.match_rom(m_rom)) return (false);
    m_owi.write(READ_SCRATCHPAD);
    return (m_owi.read(&m_scratchpad, sizeof(m_scratchpad)));
  }

  /**
   * Write the contents of the scratchpad triggers and configuration
   * (3 bytes) to device. Call with match parameter false if used with
   * search_rom().
   * @param[in] match rom code (default true).
   * @return true(1) if successful otherwise false(0).
   */
  bool write_scratchpad(bool match = true)
  {
    if (match && !m_owi.match_rom(m_rom)) return (false);
    m_owi.write(WRITE_SCRATCHPAD, &m_scratchpad.high_trigger, CONFIG_MAX);
    return (true);
  }

  /**
   * Copy device scratchpad triggers and configuration data to device
   * EEPROM. Call with match parameter false if used with search_rom().
   * @param[in] match rom code (default true).
   * @return true(1) if successful otherwise false(0).
   */
  bool copy_scratchpad(bool match = true)
  {
    if (match && !m_owi.match_rom(m_rom)) return (false);
    m_owi.write(COPY_SCRATCHPAD);
    return (true);
  }

  /**
   * Recall the alarm triggers and configuration from device EEPROM.
   * Call with match parameter false if used with search_rom().
   * @param[in] match rom code (default true).
   * @return true(1) if successful otherwise false(0).
   */
  bool recall(bool match = true)
  {
    if (match && !m_owi.match_rom(m_rom)) return (false);
    m_owi.write(RECALL_E);
    return (true);
  }

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
  } __attribute__((packed));
  scratchpad_t m_scratchpad;

  /** Size of configuration; high/low trigger and configuration byte. */
  static const uint8_t CONFIG_MAX = 3;

  /** Watchdog millis on convert_request(). */
  uint16_t m_start;

  /** Convert request pending. */
  bool m_converting;
};
#endif
