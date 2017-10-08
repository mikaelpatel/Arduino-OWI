#include "GPIO.h"
#include "OWI.h"
#include "TWI.h"
#include "Hardware/OWI.h"
#include <util/crc16.h>

#define USE_SOFTWARE_TWI
#if defined(USE_SOFTWARE_TWI)
#include "Software/TWI.h"
Software::TWI<BOARD::D18,BOARD::D19> twi;
#else
#include "Hardware/TWI.h"
Hardware::TWI twi;
#endif

Hardware::OWI owi(twi);

// DS18B20 Commands
enum {
  CONVERT_T = 0x44,
  READ_SCRATCHPAD = 0xbe
};

// DS18B20 Scratchpad Memory
struct scratchpad_t {
  int16_t temperature;
  int8_t high_trigger;
  int8_t low_trigger;
  uint8_t configuration;
  uint8_t reserved[3];
  uint8_t crc;
};

#define TRACE(expr)							\
  do {									\
    Serial.print(#expr "=");						\
    Serial.println(expr);						\
  } while (0)

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  TRACE(owi.device_reset());
  TRACE(owi.device_config());
  TRACE(owi.set_read_pointer(owi.READ_DATA_REGISTER));
  TRACE(owi.set_read_pointer(owi.CONFIGURATION_REGISTER));
  TRACE(owi.set_read_pointer(owi.CHANNEL_SELECTION_REGISTER));
  TRACE(owi.set_read_pointer(owi.STATUS_REGISTER));

  uint8_t rom[owi.ROM_MAX] = { 0 };
  uint8_t crc = 0;
  TRACE(owi.reset());
  Serial.print(F("rom="));
  owi.write(owi.READ_ROM);
  for (size_t i = 0; i < sizeof(rom); i++) {
    rom[i] = owi.read(8);
    if (rom[i] < 0x10) Serial.print(0);
    Serial.print(rom[i], HEX);
    crc = _crc_ibutton_update(crc, rom[i]);
  }
  Serial.print(F(", crc="));
  Serial.println(crc);

  uint8_t value = 0;
  uint8_t bits = 0;
  uint8_t ix = 0;
  uint8_t res = 0;
  uint8_t dir = 0;
  bool id;
  bool nid;
  crc = 0;
  TRACE(owi.reset());
  Serial.print(F("rom="));
  owi.write(owi.SEARCH_ROM);
  do {
    res = owi.triplet(dir);
    id = (res & 1) != 0;
    nid = (res & 2) != 0;
    value = (value >> 1);
    if (dir) value |= 0x80;
    bits += 1;
    if (bits == CHARBITS) {
      rom[ix] = value;
      if (rom[ix] < 0x10) Serial.print(0);
      Serial.print(rom[ix], HEX);
      crc = _crc_ibutton_update(crc, rom[ix]);
      ix += 1;
      bits = 0;
      value = 0;
    }
  } while (id != nid);
  Serial.print(F(", crc="));
  Serial.println(crc);
}

void loop()
{
  owi.reset();
  owi.write(owi.SKIP_ROM);
  owi.write(CONVERT_T);
  delay(750);

  owi.reset();
  owi.write(owi.SKIP_ROM);
  owi.write(READ_SCRATCHPAD);

  scratchpad_t scratchpad;
  uint8_t* p = (uint8_t*) &scratchpad;
  uint8_t crc = 0;
  for (size_t i = 0; i < sizeof(scratchpad); i++) {
    p[i] = owi.read();
    crc = _crc_ibutton_update(crc, p[i]);
  }
  if (crc == 0) {
    float temperature = scratchpad.temperature * 0.0625;
    TRACE(temperature);
  } else
    TRACE(crc);
  delay(2000);
}
