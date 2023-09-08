#include <settings.h>

/*------------- Fields / Vars -------------*/

// First Boot Flag
bool firstBoot = true;

bool canEnabled = true;

//Zeitstempel für Sekundentimer
unsigned long previousOneSecondTick = 0;

int lastIgnitionState = HIGH; //Hält den letzten Zündungsstatus

bool startup = true; //Steuerung ist gerade angelaufen.

bool debugMode = true; //Debugmodus aktiv?
bool debugCanMessages = false; //Debug der CAN-Kommunikation

//Geschwindigkeit der Seriellen Schnittstelle "Serial"
const int serialBaud = 115200;

//Initialstatus der eingebauten LED
int ledState = LOW;

//Motorstatus
bool engineRunning = false;

// Device Name
char deviceName[16] = "FX-Underglow";

// Security Pin
int securityPin = 123456;

// LED User Color
CRGB userColor = CRGB(255, 0, 0);
byte brightness = 128;
byte color_red = 255;
byte color_green = 255;
byte color_blue = 255;

int backLeds = 20;
int centerLeds = 100;
int frontLeds = 20;

// Led Count per strip
int ledCountPerChannel = backLeds + centerLeds + frontLeds;

int ledCount[STRIP_CHANNELS] = {ledCountPerChannel, ledCountPerChannel};

int updatesPerSecond = 60;