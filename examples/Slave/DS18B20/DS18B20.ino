#include "GPIO.h"
#include "OWI.h"
#include "Slave/OWI.h"

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
  int16_t temperature;		//!< Temperature reading (9-12 bits).
  int8_t high_trigger;		//!< High temperature trigger.
  int8_t low_trigger;		//!< Low temperature trigger.
  uint8_t configuration;	//!< Configuration; resolution, alarm.
  uint8_t reserved[3];		//!< Reserved.
} __attribute__((packed));

// ROM identity for slave device
uint8_t ROM[OWI::ROM_MAX] = {
  0x28, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
};

// The slave device
Slave::OWI<BOARD::D7> owi(ROM);

// Scratchpad with temperature, trigger and configuration
scratchpad_t scratchpad = {
  0x0550,			//!< 85 C temperature
  75,				//!< 75 C high trigger
  70,				//!< 70 C low trigger
  0x3f,				//!< 9 bits conversion
  { 0, 0, 0 }			//!< Reserved
};

// Analog pin used for emulated temperture reading
int pin = A0;

void setup()
{
}

// This sketch uses approx. 1700 bytes (Uno) of program storage space,
// and 33 bytes for global variables (random access memory)
void loop()
{
  // Application could do something in the background before
  // handling incoming commands
  if (!owi.rom_command()) return;

  // DS18B20 emulation with analog read; mapping from 0..1023
  // to -128.00..127.75 C, 10-bits resolution
  int16_t value;
  switch (owi.read()) {
  case CONVERT_T:
    value = analogRead(pin) - 512;
    scratchpad.temperature = (value << 2);
    value >>= 2;
    owi.alarm(value >= scratchpad.high_trigger ||
	      value <= scratchpad.low_trigger);
    break;
  case READ_SCRATCHPAD:
    owi.write(&scratchpad, sizeof(scratchpad));
    break;
  case WRITE_SCRATCHPAD:
    owi.read(&scratchpad.high_trigger, 3);
    break;
  }
}
