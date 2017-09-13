# Arduino-OWI
The OWI library has been developed to support the implementation of
1-wire device drivers. The library includes an example device driver
and a bus scanner.

Version: 1.2

## Classes

* [Abstract One-Wire Bus Manager and Device Interface, OWI](./src/OWI.h)
* [Software One-Wire Interface, Software::OWI](./src/Software/OWI.h)
* [Programmable Resolution 1-Wire Digital Thermometer, DS18B20](./src/Driver/DS18B20.h)

## Example Sketches

* [DS18B20](./examples/DS18B20)
* [DS1990A](./examples/DS1990A)
* [Scanner](./examples/Scanner)

## Dependencies

* [General Purpose Input/Output library for Arduino, GPIO](https://github.com/mikaelpatel/Arduino-GPIO)
