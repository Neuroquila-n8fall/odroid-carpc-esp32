#include <settings.h>

/*------------- Fields / Vars -------------*/
//Zeitstempel für Sekundentimer
unsigned long previousOneSecondTick = 0;

int lastIgnitionState = HIGH; //Hält den letzten Zündungsstatus

const int CYCLE_DELAY = 500; //Verzögerung in ms pro Schleifendurchlauf

bool startup = true; //Steuerung ist gerade angelaufen.

bool debugMode = true; //Debugmodus aktiv?
bool debugCanMessages = false; //Debug der CAN-Kommunikation

bool hibernateActive = false;

//Geschwindigkeit der Seriellen Schnittstelle "Serial"
const int serialBaud = 115200;

//Initialstatus der eingebauten LED
int ledState = LOW;

//Motorstatus
bool engineRunning = false;