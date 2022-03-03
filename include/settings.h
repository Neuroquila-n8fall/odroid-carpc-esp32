#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <Arduino.h>



/*------------- Fields / Vars -------------*/
//Zeitstempel für Sekundentimer
extern unsigned long previousOneSecondTick;

//Spannungsmessung
extern const int maxAverageReadings;
extern float voltageReadings[];
extern int voltageReadingCounter;

//Mögliche Aktionen
enum PendingAction
{
  ODROID_START,
  ODROID_STOP,
  ODROID_STANDBY,
  NONE
};
//Beinhaltet die aktuelle Aktion, welche ausgeführt wird oder werden soll.
//Sofern diese nicht NONE ist, können keine weiteren Aktionen ausgeführt werden.
//Dies soll doppelte Ausführungen von Start & Stop während der Hoch- und Herunterfahrphase des PCs verhindern
extern PendingAction pendingAction;

//Hier wird gespeichert, ob nach dem Ausführen einer Aktion eine weitere folgt
//Beispiel: Das Auto wird aufgesperrt und innerhalb des Startup-Intervalls wieder zugesperrt. Der PC würde nun eingeschaltet bleiben.
//Hier würde nun gespeichert werden, dass der PC wieder heruntergefahren werden soll, sobald der Timer abgelaufen ist.
extern PendingAction queuedAction;

//2 Sekunden für Aufwecken
extern const int ODROID_BOOT_HOLD_DELAY;
//5 Sekunden zum Herunterfahren (Lineage braucht nur 1 Sekunde bei aktivierung der Option "Shutdown without prompt")
extern const int ODROID_SHUTDOWN_HOLD_DELAY;

extern int odroidRunning; //Ergebnis vom Odroid GPIO #1. LOW = aus, HIGH = an

extern int lastIgnitionState; //Hält den letzten Zündungsstatus

extern const int CYCLE_DELAY; //Verzögerung in ms pro Schleifendurchlauf
static unsigned long currentMillis = 0L;

extern unsigned long previousOdroidActionTime;  //Vorherige Zeitmessung für Odroid Steuerung
extern unsigned long previousMainTaskTime;      //Vorherige Zeitmessung für allgemeinen Timer
extern unsigned long previousIgnitionCheckTime; //Vorherige Zeitmessung für Zündungsstatus
extern unsigned long previousOdroidPauseTime;   //Vorherige Zeitmessung für Odroid Sleepmodus

extern bool odroidStartRequested;    //Start von Odroid angefordert
extern bool odroidShutdownRequested; //Stop von Odroid angefordert
extern bool odroidPauseRequested;    //Sleep oder Wakeup angefordert

extern bool startup; //Steuerung ist gerade angelaufen.

extern bool debugMode; //Debugmodus aktiv?

extern int ignitionOn; //Zündung - HIGH = Aus, LOW = An

extern const int ODROID_STANDBY_HOLD_DELAY;       //Button Press für Display und Sleep.
extern const unsigned long WAKEUP_WAIT_DELAY;   //10 Sekunden Wartezeit für Aufwecken
extern const unsigned long STARTUP_WAIT_DELAY;  //Wartezeit für Start
extern const unsigned long SHUTDOWN_WAIT_DELAY; //Wartezeit für Herunterfahren
extern unsigned long startPowerTransitionMillis;    //Counter für den Aufweck- und Herunterfahrprozess
extern const unsigned long ODROID_STANDBY_DELAY; //Wartzeit für Sleepfunktion

extern bool hibernateActive;

//Geschwindigkeit der Seriellen Schnittstelle "Serial"
extern const int serialBaud;

//zuletzt errechneter Helligkeitswert für Display.
extern int lastBrightness;

//Stunden
extern int hours;
//Minuten
extern int minutes;
//Sekunden
extern int seconds;
//Tag
extern int days;
//Monat
extern int month;
//Jahr
extern int year;

//Initialstatus der eingebauten LED
extern int ledState;

//Displayhelligkeit

//Geringster Helligkeitswert vom Lichtsensor (Dunkelheit / abgedeckt)
extern const int MIN_LM_LIGHT_LEVEL;
//Höchster Helligkeitswert vom Lichtsensor (Direktes Sonnenlicht)
extern const int MAX_LM_LIGHT_LEVEL;

extern int VU7A_dutyCycle;
//Display Helligkeitssteuerung. PWM Einstellungen für 4103 LED Treiber
extern const int VU7A_PWMFreq;                          // 4103 LED Driver arbeitet mit bis zu 1kHz. Wir nehmen hier das was der DUE bereits geleistet hat.
extern const int VU7A_PWMChannel;
extern const int VU7A_PWMResolution;                     // DUE arbeitet mit 12 bits. Das hat immer sehr gut funktioniert.
extern const int VU7A_MAX_DUTY_CYCLE;

//Minimaler Steuerwert für Displayhelligkeit
extern const int MIN_DISPLAY_BRIGHTNESS;
//Maximaler Steuerwert für Displayhelligkeit
extern const int MAX_DISPLAY_BRIGHTNESS;

#endif