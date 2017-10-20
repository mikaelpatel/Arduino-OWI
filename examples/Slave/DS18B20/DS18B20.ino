#include "GPIO.h"
#include "OWI.h"
#include "Slave/OWI.h"

/** DS18B20 family code. */
static const uint8_t FAMILY_CODE = 0x28;

/**
 * DS18B20 Function Commands.
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
 * DS18B20 Memory Map.
 */
struct scratchpad_t {
  int16_t temperature;		//!< Temperature reading (9-12 bits).
  int8_t high_trigger;		//!< High temperature trigger.
  int8_t low_trigger;		//!< Low temperature trigger.
  uint8_t configuration;	//!< Configuration; resolution, alarm.
  uint8_t reserved[3];		//!< Reserved.
} __attribute__((packed));

// ROM identity for slave device
uint8_t ROM[OWI::ROM_MAX];

// Slave device one wire access
Slave::OWI<BOARD::D7> owi(ROM);

// Scratchpad with temperature, triggers and configuration
scratchpad_t scratchpad = {
  0x0550,			//!< 85 C default temperature,
  75,				//!< 75 C high trigger, and
  70,				//!< 70 C low trigger
  0x3f,				//!< 10 bits conversion
  { 0, 0, 0 }			//!< Reserved
};

// Analog pin used for emulated temperature reading
const int pin = A0;

void setup()
{
  // Random ROM identity code
  ROM[0] = FAMILY_CODE;
  uint8_t* p = (uint8_t*) 0;
  for (size_t i = 1; i < OWI::ROM_MAX; i++) ROM[i] = *p++;
}

// This sketch uses approx. 1900 bytes (Uno) of program storage space,
// and 38 bytes for global variables (random access memory)
void loop()
{
  // Application could do something in the background before
  // handling incoming commands
  if (!owi.rom_command()) return;

  // DS18B20 emulation with analog read; mapping from 0..1023
  // to -128.00..127.75 C, 10-bits resolution. Set alarm according
  // to low and high thresholds
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
  case COPY_SCRATCHPAD:
  case RECALL_E:
  case READ_POWER_SUPPLY:
    break;
  }
}
