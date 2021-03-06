# Joba_Tsl2561

This is an Arduino library for the TSL2561 digital luminosity sensors from Ams (Taos).

## Design goals
* It is modularized so you can use only what you need if space/ram is constrained.
* It does not swallow error codes so you can react on them.
* It doesn't use floats as they are overkill for most IoT stuff.

## Datasheet used
http://ams.com/eng/Products/Light-Sensors/Ambient-Light-Sensors/TSL2561/TSL2560-TSL2561-Datasheet
http://ams.com/eng/content/download/250094/975485/file/TSL2560-61_DS000110_2-00.pdf
http://ams.com/eng/content/view/download/145438
http://ams.com/eng/content/view/download/181895

## Usage
To use the library, just place the folder in your projects lib folder.
For usage, see the examples folder.

## Design
The library has 3 classes:
Tsl2561     All register access as described in the datasheet, except for interrupts
Tsl2561Util Convenience functions like lux calculation or automatic gain
Tsl2561Int  TODO, Interrupt related stuff (not needed if int pin is not connected)

## Compatibility
Tested with boards Nano, ESP8266 and ESP32 iArduino IDE and PlatformIO

Joachim Banzhaf
