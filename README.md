# odroid-carpc-esp32
An ESP32 project for controlling an Odroid N2 via BMW Keyfob and iDrive.

## Hardware used
- BMW 1-Series 2009
- ESP32 Devkit
- iDrive 7-Button Controller 

## Purpose
This project is intended to support an Odroid N2 in its function as a car-pc. This also involves controlling the display brightness of the Vu7+ display by applying PWM signals to its backlight chip by reacting on the cars' light sensor on the windscreen.
As a bonus it's designed to support an iDrive controller as an input device for the Odroid to have comfortable access to the car-pc functions. To make it function properly an app called "Button Mapper" is used to react on F-Keys.
See: https://play.google.com/store/apps/details?id=flar2.homebutton

## 3rd Party Libraries used
- NimBLE-Arduino
- erropix/ESP32 AnalogWrite@^0.2
- adafruit/Adafruit INA219@^1.1.1
- t-vk/ESP32 BLE Keyboard@0.3.1
- miwagner/ESP32CAN@^0.0.1

## Usage
### Wiring
W.I.P.
### Parameters
W.I.P.

## Known Issues
- Nothing as of now.

## Questions I got asked...
- Why?
Because I always felt the need to include a full blown car pc into my car but the original iDrive system is just too limited (and quite expensive). I want a browsable music library with cloud sync, internet, the precious Torque app, ...all the good stuff!
The icing on the cake was the decision to go the extra mile and swap the center console and install an iDrive 7-Button control unit. It looks like OEM but it does much, much more.

