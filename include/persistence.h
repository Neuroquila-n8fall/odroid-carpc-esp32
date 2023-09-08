#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <preferences.h>


extern Preferences preferences;

extern void readFirstBoot();
extern void writeFirstBoot();

extern void readLedProfile();
extern void writeLedProfile();

extern void readLedBrightness();
extern void writeLedBrightness();

extern void readLedColor();
extern void writeLedColor();

extern void readSecurityPin();
extern void writeSecurityPin();

extern void readDeviceName();
extern void writeDeviceName();

extern void readLedCount();
extern void writeLedCount();

extern void updateLedChannels();

extern void readUpdatesPerSecond();
extern void writeUpdatesPerSecond();

#endif