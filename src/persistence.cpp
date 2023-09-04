#include "persistence.h"
#include "led-control.h"
// Path: src/persistence.cpp

Preferences preferences;

// LED User Preferences

//Active Profile
void readLedProfile() {
    preferences.begin("activeProfile", true); // Open in read-only mode
    int32_t storedProfile = preferences.getInt("activeProfile", 0);
    activeProfile = static_cast<Profiles>(storedProfile);
    preferences.end();
}

void writeLedProfile() {
    preferences.begin("activeProfile", false);
    preferences.putInt("activeProfile", activeProfile);
    preferences.end();
}

//LED Brightness

void readLedBrightness() {
    preferences.begin("ledBrightness", true);
    brightness = preferences.getInt("ledBrightness", 0);
}

void writeLedBrightness() {
    preferences.begin("ledBrightness", false);
    preferences.putInt("ledBrightness", brightness);
    preferences.end();
}

//Single LED Color

void readLedColor() {
    preferences.begin("ledColor", true);
    color_red = preferences.getUChar("ledColor_red", 0);
    color_green = preferences.getUChar("ledColor_green", 0);
    color_blue = preferences.getUChar("ledColor_blue", 0);
    preferences.end();
}

void writeLedColor() {
    preferences.begin("ledColor", false);
    preferences.putUChar("ledColor_red", color_red);
    preferences.putUChar("ledColor_green", color_green);
    preferences.putUChar("ledColor_blue", color_blue);
    preferences.end();
}

