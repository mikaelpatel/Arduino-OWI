# Arduino-OWI
The OWI library has been developed to support the implementation of
1-wire bus managers and device drivers. The library includes a GPIO
based software and DS2482 based hardware bus manager, Slave device
support class, and device driver for DS18B20.

The examples directory contains device search, bus scanner,
thermometer alarm search, and example sketches for DS18B20 and
DS1990A.

Version: 1.8

## Classes

* [Abstract One-Wire Bus Manager and Device Interface, OWI](./src/OWI.h)
* [Software One-Wire Bus Manager, GPIO, Software::OWI](./src/Software/OWI.h)
* [Hardware One-Wire Bus Manager, DS2482, Hardware::OWI](./src/Hardware/OWI.h)
* [Software One-Wire Slave Device, Slave::OWI](./src/Slave/OWI.h)
* [Programmable Resolution 1-Wire Digital Thermometer, DS18B20](./src/Driver/DS18B20.h)
* [One-Wire Remove Arduino, Master](./src/Driver/Arduino.h)

## Example Sketches

* [Alarm](./examples/Alarm)
* [Arduino](./examples/Arduino)
* [Arduino, Slave](./examples/Slave/Arduino)
* [Search](./examples/Search)
* [Scanner](./examples/Scanner)
* [DS18B20](./examples/DS18B20)
* [DS18B20, Slave](./examples/Slave/DS18B20)
* [DS1990A](./examples/DS1990A)

[ATtiny](./examples/ATtiny) and [DS2482](./examples/DS2482)
variants.

## Dependencies

* [Arduino-GPIO](https://github.com/mikaelpatel/Arduino-GPIO)
* [Arduino-TWI](https://github.com/mikaelpatel/Arduino-TWI)
