# Arduino-OWI
The OWI library has been developed to support the implementation of
1-wire bus managers and device drivers. The library includes a GPIO
based software and DS2482 based hardware bus manager, and device
driver for DS18B20.

The examples directory contains device search, bus scanner,
thermometer alarm search, and example sketches for DS18B20, DS1990A
and DS2482.

Version: 1.6

## Classes

* [Abstract One-Wire Bus Manager and Device Interface, OWI](./src/OWI.h)
* [Software One-Wire Bus Manager, GPIO, Software::OWI](./src/Software/OWI.h)
* [Hardware One-Wire Bus Manager, DS2482, Hardware::OWI](./src/Hardware/OWI.h)
* [Programmable Resolution 1-Wire Digital Thermometer, DS18B20](./src/Driver/DS18B20.h)

## Example Sketches

* [DS18B20](./examples/DS18B20)
* [DS1990A](./examples/DS1990A)
* [DS2482](./examples/DS2482)
* [Alarm](./examples/Alarm)
* [Scanner](./examples/Scanner)
* [Search](./examples/Search)

## Dependencies

* [Arduino-GPIO](https://github.com/mikaelpatel/Arduino-GPIO)
* [Arduino-TWI](https://github.com/mikaelpatel/Arduino-TWI)
