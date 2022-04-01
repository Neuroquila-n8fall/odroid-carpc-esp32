#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <Arduino.h>



/*------------- Fields / Vars -------------*/
//Zeitstempel für Sekundentimer
extern unsigned long previousOneSecondTick;


extern const int CYCLE_DELAY; //Verzögerung in ms pro Schleifendurchlauf
static unsigned long currentMillis;

extern bool debugMode; //Debugmodus aktiv?
extern bool debugCanMessages; //Debug der CAN-Kommunikation

extern bool hibernateActive;

//Geschwindigkeit der Seriellen Schnittstelle "Serial"
extern const int serialBaud;

//Initialstatus der eingebauten LED
extern int ledState;

#endif