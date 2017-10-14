#include "GPIO.h"
#include "OWI.h"
#include "benchmark.h"

// Configure: Software/Hardware OWI Bus Manager
#define USE_SOFTWARE_OWI
#if defined(USE_SOFTWARE_OWI)
#include "Software/OWI.h"
Software::OWI<BOARD::D7> owi;

#else
#include "TWI.h"
#include "Hardware/OWI.h"
// Configure: Software/Hardware TWI Bus Manager
// #define USE_SOFTWARE_TWI
#if defined(USE_SOFTWARE_TWI)
#include "Software/TWI.h"
#if defined(SAM)
Software::TWI<BOARD::D8,BOARD::D9> twi;
#else
Software::TWI<BOARD::D18,BOARD::D19> twi;
#endif
#else
#include "Hardware/TWI.h"
Hardware::TWI twi;
#endif
Hardware::OWI owi(twi);
#endif

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  BENCHMARK_BASELINE(1);
}

void loop()
{
  // Measure the standard 1-wire bus manager functions
  // All measurements are in micro-seconds

  uint8_t rom[owi.ROM_MAX];
  uint8_t dir = 0;

  // Cyclic Redundancy Check per byte
  MEASURE(owi.crc_update(0,0));

  // Reset pulse and presence check
  MEASURE(owi.reset());

  // Write bit and byte
  MEASURE(owi.write(0,1));
  MEASURE(owi.write(1,1));
  MEASURE(owi.write(0x00));
  MEASURE(owi.write(0xff));

  // Read bit and byte
  MEASURE(owi.read(1));
  MEASURE(owi.read());

  // Triplet; read two bits and write one
  MEASURE(owi.triplet(dir));

  // Standard rom functions
  MEASURE(owi.read_rom(rom));
  MEASURE(owi.skip_rom());
  MEASURE(owi.match_rom(rom));
  MEASURE(owi.search_rom(0, rom));
  MEASURE(owi.alarm_search(rom));

  Serial.println();
  delay(2000);
}
