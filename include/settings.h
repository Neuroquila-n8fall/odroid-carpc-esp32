#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <Arduino.h>
#include <FastLED.h>

/*------------- Fields / Vars -------------*/

// First Boot Flag
extern bool firstBoot;

extern bool canEnabled;

extern char deviceName[16];
extern int securityPin;

extern bool debugMode; //Debugmodus aktiv?
extern bool debugCanMessages; //Debug der CAN-Kommunikation

//Geschwindigkeit der Seriellen Schnittstelle "Serial"
extern const int serialBaud;

//Initialstatus der eingebauten LED
extern int ledState;

// Strip Channel 1
const int CHANNEL_1_PIN = 25;

// Strip Channel 2
const int CHANNEL_2_PIN = 26;

// Strip Channel 3
const int CHANNEL_3_PIN = 14;

const int STRIP_CHANNELS = 2;
const int MAX_PIXELS_PER_STRIP = 2048;

extern CRGB userColor;
extern byte brightness;
extern byte color_red;
extern byte color_green;
extern byte color_blue;

extern int backLeds;
extern int centerLeds;
extern int frontLeds;

// Led Count per strip
extern int ledCount[STRIP_CHANNELS];

// Updates Per Second "FPS" so to speak
extern int updatesPerSecond;

#endif