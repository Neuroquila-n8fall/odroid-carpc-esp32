#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <preferences.h>


extern Preferences preferences;
extern void readLedProfile();
extern void writeLedProfile();
extern void readLedBrightness();
extern void writeLedBrightness();
extern void readLedColor();
extern void writeLedColor();

#endif