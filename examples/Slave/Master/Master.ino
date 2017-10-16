#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"

Software::OWI<BOARD::D7> owi;

// Extended commands
enum {
  PIN_MODE = 0x11,
  DIGITAL_READ = 0x22,
  DIGITAL_WRITE = 0x33,
  ANALOG_READ = 0x44,
  ANALOG_WRITE = 0x55
};

struct res_t {
  uint16_t value;
  uint8_t crc;
} __attribute__((packed));

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  static uint32_t nr = 0;
  static uint16_t err = 0;
  uint8_t ROM[OWI::ROM_MAX];

  // read_rom:uint8_t res[ROM_MAX]
  if (owi.read_rom(ROM)) {
    Serial.print(nr++);
    Serial.print(':');
    Serial.print(err);
    Serial.print(F(":read_rom:rom="));
    for (size_t i = 0; i < sizeof(ROM) - 1; i++) {
      if (ROM[i] < 0x10) Serial.print('0');
      Serial.print(ROM[i], HEX);
      if (i < sizeof(ROM) - 2) Serial.print(',');
    }
    Serial.print(F(",crc="));
    uint8_t crc = ROM[sizeof(ROM) - 1];
    if (crc < 0x10) Serial.print('0');
    Serial.print(crc, HEX);
    Serial.println();
  }
  else {
    err++;
  }

  // match_rom(uint8_t[ROM_MAX]):analog_read(pin):
  // struct { uint16_t res; uint8_t crc } res
  if (owi.match_rom(ROM)) {
    uint8_t pin = 0;
    res_t res;
    owi.write(ANALOG_READ);
    owi.write(pin);
    delayMicroseconds(200);
    if (owi.read(&res, sizeof(res))) {
      Serial.print(nr++);
      Serial.print(':');
      Serial.print(err);
      Serial.print(F(":match_rom:analog_read:pin="));
      Serial.print(pin);
      Serial.print(':');
      Serial.println(res.value);
    }
    else {
      err++;
    }
  }
  else {
    err++;
  }

  // skip_rom:digital_read(pin):uint8_t res:1
  if (owi.skip_rom()) {
    uint8_t pin = 8;
    uint8_t value = 0;
    owi.write(DIGITAL_READ);
    owi.write(pin);
    value = owi.read(1);
    Serial.print(nr++);
    Serial.print(':');
    Serial.print(err);
    Serial.print(F(":skip_rom:digital_read:pin="));
    Serial.print(pin);
    Serial.print(':');
    Serial.println(value);
  }
  else {
    err++;
  }

  int8_t last = owi.FIRST;
  uint8_t id = 0;
  memset(ROM, 0, sizeof(ROM));
  do {
    last = owi.search_rom(1, ROM, last);
    if (last == owi.ERROR) break;
    uint8_t pin = 13;
    static uint8_t value = 0;
    owi.write(DIGITAL_WRITE);
    owi.write(pin);
    owi.write(value = !value);
    Serial.print(nr++);
    Serial.print(':');
    Serial.print(err);
    Serial.print(F(":search_rom["));
    Serial.print(id++);
    Serial.print(F("]:rom="));
    for (size_t i = 0; i < sizeof(ROM) - 1; i++) {
      if (ROM[i] < 0x10) Serial.print('0');
      Serial.print(ROM[i], HEX);
      if (i < sizeof(ROM) - 2) Serial.print(',');
    }
    Serial.print(F(",crc="));
    uint8_t crc = ROM[sizeof(ROM) - 1];
    if (crc < 0x10) Serial.print('0');
    Serial.print(crc, HEX);
    Serial.print(F(":digital_write:pin="));
    Serial.print(pin);
    Serial.print(':');
    Serial.println(value);
  } while (last != owi.LAST);

  last = owi.FIRST;
  id = 0;
  memset(ROM, 0, sizeof(ROM));
  do {
    last = owi.alarm_search(ROM, last);
    if (last == owi.ERROR) break;
    static uint16_t alarms = 0;
    owi.write(0);
    Serial.print(nr++);
    Serial.print(':');
    Serial.print(err);
    Serial.print(':');
    Serial.print(++alarms);
    Serial.print(F(":alarm_search["));
    Serial.print(id++);
    Serial.print(F("]:rom="));
    for (size_t i = 0; i < sizeof(ROM) - 1; i++) {
      if (ROM[i] < 0x10) Serial.print('0');
      Serial.print(ROM[i], HEX);
      if (i < sizeof(ROM) - 2) Serial.print(',');
    }
    Serial.print(F(",crc="));
    uint8_t crc = ROM[sizeof(ROM) - 1];
    if (crc < 0x10) Serial.print('0');
    Serial.print(crc, HEX);
    Serial.println();
  } while (last != owi.LAST);
  Serial.println();
  delay(100 + rand() % 300);
}
