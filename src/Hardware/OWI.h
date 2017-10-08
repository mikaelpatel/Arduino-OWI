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
#include "TWI.h"

/**
 * One Wire Interface (OWI) Bus Manager class using DS2482,
 * Single-Channel 1-Wire Master, TWI to OWI Bridge Device.
 */
namespace Hardware {
class OWI : public ::OWI, protected TWI::Device {
public:
  /**
   * Construct one wire bus manager for DS2482.
   * @param[in] twi bus manager.
   * @param[in] subaddr sub-address for device.
   */
  OWI(TWI& twi, uint8_t subaddr = 0) :
    TWI::Device(twi, 0x18 | (subaddr & 0x03))
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
    status_t status;
    uint8_t cmd;
    int count;

    // Issue one wire reset command
    cmd = ONE_WIRE_RESET;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(&cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }
    if (!TWI::Device::release()) return (false);
    return ((count == sizeof(status)) && status.PPD);

  error:
    TWI::Device::release();
    return (false);
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
    status_t status;
    uint8_t cmd;
    int count;

    // Check for bit read
    if (bits != CHARBITS) {
      uint8_t adjust = CHARBITS - bits;
      uint8_t res = 0;
      while (bits--) {
	res >>= 1;
	if (read_bit()) res |= 0x80;
      }
      res >>= adjust;
      return (res);
    }

    // Issue one wire read byte command
    cmd = ONE_WIRE_READ_BYTE;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(&cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }
    if (!TWI::Device::release()) return (false);
    if ((count != sizeof(status)) || status.IWB) return (0);

    // Read data register value
    return (set_read_pointer(READ_DATA_REGISTER));

  error:
    TWI::Device::release();
    return (0);
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
    status_t status;
    uint8_t cmd[2];
    int count;

    // Issue one wire write byte command with given data
    if (bits != CHARBITS) {
      while (bits--) {
	write_bit(value & 0x01);
	value >>= 1;
      }
      return;
    }

    cmd[0] = ONE_WIRE_WRITE_BYTE;
    cmd[1] = value;
    if (!TWI::Device::acquire()) return;
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }

  error:
    TWI::Device::release();
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
    status_t status;
    uint8_t cmd[2];
    int count;

    // Issue one wire single bit command with given data
    cmd[0] = ONE_WIRE_TRIPLET;
    cmd[1] = (dir ? 0x80 : 0x00);
    if (!TWI::Device::acquire()) return (ERROR);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }
    if (!TWI::Device::release()) return (ERROR);
    if (count != sizeof(status) && status.IWB) return (ERROR);
    dir = status.DIR;
    return ((status >> 5) & 0x3);

  error:
    TWI::Device::release();
    return (ERROR);
  }

  /**
   * Global reset of device state machine logic. Returns true if
   * successful otherwise false.
   * @return bool.
   */
  bool device_reset()
  {
    status_t status;
    uint8_t cmd;
    int count;

    // Issue device reset command
    cmd = DEVICE_RESET;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(&cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Check status register for device reset
    count = TWI::Device::read(&status, sizeof(status));
    if (!TWI::Device::release()) return (false);
    return ((count == sizeof(status)) && status.RST);

  error:
    TWI::Device::release();
    return (false);
  }

  /**
   * Configure one wire bus master with given parameters. Returns true
   * if successful otherwise false.
   * @param[in] apu active pull-up (default true).
   * @param[in] spu strong pull-up (default false).
   * @param[in] iws one wire speed (default false).
   * @return bool.
   */
  bool device_config(bool apu = true, bool spu = false, bool iws = false)
  {
    config_t config;
    status_t status;
    uint8_t cmd[2];
    int count;

    // Set configuration bit-fields
    config.APU = apu;
    config.SPU = spu;
    config.IWS = iws;
    config.COMP = ~config;

    // Issue write configuration command with given setting
    cmd[0] = WRITE_CONGIFURATION;
    cmd[1] = config;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Read status and check configuration
    count = TWI::Device::read(&status, sizeof(status));
    if (!TWI::Device::release()) return (false);
    return ((count == sizeof(status)) && !status.RST);

  error:
    TWI::Device::release();
    return (false);
  }

  /**
   * Device Registers, pp. 5. Valid Pointer Codes, pp. 10.
   */
  enum Register {
    STATUS_REGISTER = 0xf0,
    READ_DATA_REGISTER = 0xe1,
    CHANNEL_SELECTION_REGISTER = 0xd2,
    CONFIGURATION_REGISTER = 0xc3
  } __attribute__((packed));

  /**
   * Set the read pointer to the specified register. Return register
   * value or negative error code.
   * @param[in] addr register address.
   * @return register value or negative error code.
   */
  int set_read_pointer(Register addr)
  {
    uint8_t cmd[2];
    uint8_t reg;
    int count;

    // Issue set read pointer command with given pointer
    cmd[0] = SET_READ_POINTER;
    cmd[1] = (uint8_t) addr;
    if (!TWI::Device::acquire()) return (-1);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Read register value
    count = TWI::Device::read(&reg, sizeof(reg));
    if (!TWI::Device::release()) return (false);
    return ((count == sizeof(reg)) ? reg : -1);

  error:
    TWI::Device::release();
    return (-1);
  }

  /**
   * Select given channel (DS2482-800). Return true if successful
   * otherwise false.
   * @param[in] chan channel number (0..7).
   * @return bool.
   */
  bool channel_select(uint8_t chan)
  {
    uint8_t cmd[2];
    int count;

    // Check channel number
    if (chan > 7) return (false);

    // Issue channel select command with channel code
    cmd[0] = CHANNEL_SELECT;
    cmd[1] = (~chan << 4) | chan;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (!TWI::Device::release()) return (false);
    return (count == sizeof(cmd));
  }

protected:
  /**
   * Function Commands, pp. 9-15.
   */
  enum {
    DEVICE_RESET = 0xf0,	//!< Device Reset.
    SET_READ_POINTER = 0xe1,	//!< Set Read Pointer.
    WRITE_CONGIFURATION = 0xd2,	//!< Write Configuration.
    CHANNEL_SELECT = 0xc3,	//!< Channel Select.
    ONE_WIRE_RESET = 0xb4,	//!< 1-Wire Reset.
    ONE_WIRE_SINGLE_BIT = 0x87,	//!< 1-Wire Single Bit.
    ONE_WIRE_WRITE_BYTE = 0xa5,	//!< 1-Wire Write Byte.
    ONE_WIRE_READ_BYTE = 0x96,	//!< 1-Wire Read Byte.
    ONE_WIRE_TRIPLET = 0x78	//!< 1-Wire Triplet.
  } __attribute__((packed));

  /**
   * Status Register, bit-fields, pp. 8-9.
   */
  union status_t {
    uint8_t as_uint8;		//!< Unsigned byte access.
    struct {			//!< Bitfield access (little endian).
      uint8_t IWB:1;		//!< 1-Wire Busy.
      uint8_t PPD:1;		//!< Presence-Pulse Detect.
      uint8_t SD:1;		//!< Short Detected.
      uint8_t LL:1;		//!< Logic Level.
      uint8_t RST:1;		//!< Device Reset.
      uint8_t SBR:1;		//!< Single Bit Result.
      uint8_t TSB:1;		//!< Triplet Second Bit.
      uint8_t DIR:1;		//!< Branch Direction Taken.
    };
    operator uint8_t()
    {
      return (as_uint8);
    }
  };

  /**
   * Configuration Register, bit-fields, pp. 5-6.
   */
  union config_t {
    uint8_t as_uint8;		//!< Unsigned byte access.
    struct {			//!< Bitfield access (little endian).
      uint8_t APU:1;		//!< Active Pullup.
      uint8_t ZERO:1;		//!< Always Zero(0).
      uint8_t SPU:1;		//!< Strong Pullup.
      uint8_t IWS:1;		//!< 1-Wire Speed.
      uint8_t COMP:4;		//!< Complement of lower 4-bits.
    };
    operator uint8_t()
    {
      return (as_uint8);
    }
    config_t()
    {
      as_uint8 = 0;
    }
  };

  /** Number of one-wire polls */
  static const int POLL_MAX = 20;

  /**
   * Read a single bit from one wire bus. Returns bit value (0 or 1)
   * or a negative error code.
   * @return bit read.
   */
  bool read_bit()
  {
    status_t status;
    uint8_t cmd[2];
    int count;

    // Issue one wire single bit command with read data time slot
    cmd[0] = ONE_WIRE_SINGLE_BIT;
    cmd[1] = 0x80;
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }
    if (!TWI::Device::release()) return (false);
    if (count != sizeof(status) || status.IWB) return (false);
    return (status.SBR);

  error:
    TWI::Device::release();
    return (false);
  }

  /**
   * Write a single bit to one wire bus. Returns true if successful
   * otherwise false.
   * @param[in] value bit to write.
   * @return bool.
   */
  bool write_bit(bool value)
  {
    status_t status;
    uint8_t cmd[2];
    int count;

    // Issue one wire single bit command with given data
    cmd[0] = ONE_WIRE_SINGLE_BIT;
    cmd[1] = (value ? 0x80 : 0x00);
    if (!TWI::Device::acquire()) return (false);
    count = TWI::Device::write(cmd, sizeof(cmd));
    if (count != sizeof(cmd)) goto error;

    // Wait for one wire operation to complete
    for (int i = 0; i < POLL_MAX; i++) {
      count = TWI::Device::read(&status, sizeof(status));
      if (count == sizeof(status) && !status.IWB) break;
    }
    if (!TWI::Device::release()) return (false);
    return ((count == sizeof(status)) && !status.IWB);

  error:
    TWI::Device::release();
    return (false);
  }
};
};
#endif
