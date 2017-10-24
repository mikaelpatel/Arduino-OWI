#include "GPIO.h"
#include "OWI.h"
#include "Software/OWI.h"
#include "assert.h"

Software::OWI<BOARD::D7> owi;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  // The Standard ROM functions:
  // 1. Read ROM from single device on the bus.
  // 2. Skip ROM to address all devices
  // 3. Match ROM to address a specific device.
  // 4. Search ROM to iterate devices.
  // 5. Alarm Search to iterate devices with alarm set.
  // 6. Match LABEL to use short addressing (Extended function).

  uint8_t ROM[OWI::ROM_MAX];
  uint8_t label;
  int8_t last;
  uint8_t id;
  bool res;

  // Read ROM from slave, and print
  ASSERT(owi.read_rom(ROM));
  Serial.print(F("read_rom:"));
  print(ROM);
  Serial.println();

  // Address slave with skip rom
  ASSERT(res = owi.skip_rom());
  Serial.print(F("skip_rom:res="));
  Serial.println(res);
  owi.write(0);

  // Address slave with match rom
  ASSERT(res = owi.match_rom(ROM));
  Serial.print(F("match_rom:res="));
  Serial.println(res);
  owi.write(0);

  // Address slave with search rom
  id = 0;
  last = owi.FIRST;
  memset(ROM, 0, sizeof(ROM));
  do {
    last = owi.search_rom(1, ROM, last);
    if (last == owi.ERROR) break;
    label = 10;
    owi.write(0x15);
    owi.write(label);
    Serial.print(F("search_rom["));
    Serial.print(id);
    Serial.print(F("]:"));
    print(ROM);
    Serial.print(F(":label_rom:label="));
    Serial.println(label);
    id += 1;
  } while (last != owi.LAST);

  // Address slave with alarm search
  last = owi.FIRST;
  id = 0;
  memset(ROM, 0, sizeof(ROM));
  do {
    last = owi.alarm_search(ROM, last);
    if (last == owi.ERROR) break;
    owi.write(0x16);
    label = owi.read();
    Serial.print(F("alarm_search["));
    Serial.print(id++);
    Serial.print(F("]:"));
    print(ROM);
    Serial.print(F(":read_label:label="));
    Serial.println(label);
  } while (last != owi.LAST);

  // Address slave with match label
  ASSERT(res = owi.match_label(10));
  owi.write(0);
  Serial.print(F("match_label:res="));
  Serial.println(res);
  Serial.println();

  delay(2000);
}

void print(uint8_t* rom)
{
  // Print 1-Wire ROM identity code; family, serial number and crc
  Serial.print(F("famly="));
  if (rom[0] < 0x10) Serial.print('0');
  Serial.print(rom[0], HEX);
  Serial.print(F(",sn="));
  for (size_t i = 1; i < OWI::ROM_MAX - 1; i++) {
    if (rom[i] < 0x10) Serial.print('0');
    Serial.print(rom[i], HEX);
    if (i < OWI::ROM_MAX - 2) Serial.print(',');
  }
  Serial.print(F(",crc="));
  uint8_t crc = rom[OWI::ROM_MAX - 1];
  if (crc < 0x10) Serial.print('0');
  Serial.print(crc, HEX);
}
