#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include <analogWrite.h>

/*------------- Functions -------------*/
//Status der Zündung abrufen und entsprechende Aktionen auslösen
extern void checkIgnitionState();
//Start bzw. Aufwecken
extern void startOdroid();
//Sofortiges geordnetes Herunterfahren
extern void stopOdroid();
//Odroid in Sleep versetzen und Display ausschalten
extern void pauseOdroid();
//Ein- und Ausgänge prüfen
extern void checkPins();
//Zeitstempel bauen
extern void buildtimeStamp();
//Mausrad simulieren, je nachdem in welche Richtung der iDrive Knopf gedreht wurde.
extern void scrollScreen();
//Uhrzeit pflegen. Ist ausschließlich dazu da die Uhrzeit voran schreiten zu lassen, wenn der Canbus inaktiv ist und keine Zeit vom Auto kommt.
extern void timeKeeper();
//Taste drücken und sofort wieder freigeben
extern void sendKey(uint8_t keycode);
//Interaktion mit serieller Konsole
extern void readConsole();
//CAN Nachricht senden.
extern bool sendMessage(int address, byte len, const uint8_t *buf);
//Power Saver
extern void enterPowerSaving();
//Exit Power Saving
extern void exitPowerSaving();
//Returns if any pending actions are queued
extern bool anyPendingActions();
//Checks if voltage leves are within bounds and executes actions accordingly
extern void checkVoltage();
//Set Display Brightness
extern void setDisplayBrightness(int value);


#endif