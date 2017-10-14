/**
 * @file OWI.h
 * @version 1.2
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

#ifndef CHARBITS
#define CHARBITS 8
#endif

/**
 * One Wire Interface (OWI) Bus Manager abstract class.
 */
class OWI {
public:
  /** One Wire device identity ROM size in bytes. */
  static const size_t ROM_MAX = 8;

  /** One Wire device identity ROM size in bits. */
  static const size_t ROMBITS = ROM_MAX * CHARBITS;

  /**
   * @override{OWI}
   * Reset the one wire bus and check that at least one device is
   * presence.
   * @return true(1) if successful otherwise false(0).
   */
  virtual bool reset() = 0;

  /**
   * @override{OWI}
   * Read the given number of bits from the one wire bus. Default
   * number of bits is CHARBITS (8).
   * @param[in] bits to be read (default CHARBITS).
   * @return value read.
   */
  virtual uint8_t read(uint8_t bits = CHARBITS) = 0;

  /**
   * Read given number of bytes from one wire bus (device) to given
   * buffer. Calculates 8-bit Cyclic Redundancy Check sum and return
   * result of check.
   * @param[in] buf buffer pointer.
   * @param[in] count number of bytes to read.
   * @return true(1) if check sum is correct otherwise false(0).
   */
  bool read(void* buf, size_t count)
  {
    uint8_t* bp = (uint8_t*) buf;
    uint8_t crc = 0;
    while (count--) {
      uint8_t value = read();
      *bp++ = value;
      crc = crc_update(crc, value);
    }
    return (crc == 0);
  }

  /**
   * @override{OWI}
   * Write the given value to the one wire bus. The bits are written
   * from LSB to MSB.
   * @param[in] value to write.
   * @param[in] bits to be written (default CHARBITS).
   */
  virtual void write(uint8_t value, uint8_t bits = CHARBITS) = 0;

  /**
   * Write the given command and given number of bytes from buffer to
   * the one wire bus (device).
   * @param[in] cmd command to write.
   * @param[in] buf buffer pointer.
   * @param[in] count number of bytes to write.
   */
  void write(uint8_t cmd, const void* buf, size_t count)
  {
    write(cmd);
    const uint8_t* bp = (const uint8_t*) buf;
    while (count--) write(*bp++);
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
    switch (read(2)) {
    case 0b00:
      write(dir, 1);
      return (0b00);
    case 0b01:
      write(1, 1);
      dir = 1;
      return (0b01);
    case 0b10:
      write(0, 1);
      dir = 0;
      return (0b10);
    default:
      return (0b11);
    }
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
   * Optimized Dallas/Maxim iButton 8-bit Cyclic Redundancy Check
   * calculation. Polynomial: x^8 + x^5 + x^4 + 1 (0x8C).
   * See http://www.maxim-ic.com/appnotes.cfm/appnote_number/27
   */
  static uint8_t crc_update(uint8_t crc, uint8_t data)
  {
    crc = crc ^ data;
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x01)
	crc = (crc >> 1) ^ 0x8C;
      else
	crc >>= 1;
    }
    return (crc);
  }

  /** Search position and return values. */
  enum {
    FIRST = -1,			//!< Start position of search.
    ERROR = -1,			//!< Error during search.
    LAST = ROMBITS		//!< Last position, search completed.
  } __attribute__((packed));

  /**
   * Search device rom given the last position of discrepancy.
   * Return position of difference or negative error code.
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

  /**
   * One-Wire Interface (OWI) Device Driver abstract class.
   */
  class Device {
  public:
    /**
     * Construct One-Wire Interface (OWI) Device Driver with given bus
     * and device address.
     * @param[in] owi bus manager.
     * @param[in] rom code (default NULL).
     */
    Device(OWI& owi, const uint8_t* rom = NULL) :
      m_owi(owi)
    {
      if (rom != NULL) memcpy(m_rom, rom, ROM_MAX);
    }

    /**
     * Set device rom code.
     * @param[in] rom code.
     */
    void rom(const uint8_t* rom)
    {
      memcpy(m_rom, rom, ROM_MAX);
    }

    /**
     * Set device rom code.
     * @param[in] rom code in program memory.
     */
    void rom_P(const uint8_t* rom)
    {
      memcpy_P(m_rom, rom, ROM_MAX);
    }

    /**
     * Get device rom code.
     * @return rom code.
     */
    uint8_t* rom()
    {
      return (m_rom);
    }

  protected:
    /** One-Wire Bus Manager. */
    OWI& m_owi;

    /** Device rom idenity code. */
    uint8_t m_rom[ROM_MAX];
  };

protected:
  /** Maximum number of reset retries. */
  static const uint8_t RESET_RETRY_MAX = 4;

  /**
   * Search device rom given the last position of discrepancy and
   * partial or full rom code.
   * @param[in] code device identity rom.
   * @param[in] last position of discrepancy (default FIRST).
   * @return position of difference or negative error code.
   */
  int8_t search(uint8_t* code, int8_t last = FIRST)
  {
    uint8_t pos = 0;
    int8_t next = LAST;
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t data = 0;
      for (uint8_t j = 0; j < 8; j++) {
	uint8_t dir = (pos == last) || ((pos < last) && (code[i] & (1 << j)));
	switch (triplet(dir)) {
	case 0b00:
	  if (pos == last)
	    last = FIRST;
	  else if (pos > last || (code[i] & (1 << j)) == 0)
	    next = pos;
	  break;
	case 0b11:
	  return (ERROR);
	}
	data >>= 1;
	if (dir) data |= 0x80;
	pos += 1;
      }
      code[i] = data;
    }
    return (next);
  }
};
#endif
