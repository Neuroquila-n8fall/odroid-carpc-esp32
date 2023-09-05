#include "persistence.h"
#include "led-control.h"
#include "settings.h"
// Path: src/persistence.cpp

Preferences preferences;

// General Controller Settings

// Controller Name / BLE Adverstised Name
void readDeviceName() {
    preferences.begin("deviceName", true); // Open in read-only mode
    preferences.getString("deviceName", deviceName, 16);
    preferences.end();
}

void writeDeviceName() {
    preferences.begin("deviceName", false);
    preferences.putString("deviceName", deviceName);
    preferences.end();
}

// BLE Security Pin

void readSecurityPin() {
    preferences.begin("securityPin", true);
    securityPin = preferences.getInt("securityPin", 0);
    preferences.end();
}

void writeSecurityPin() {
    preferences.begin("securityPin", false);
    preferences.putInt("securityPin", securityPin);
    preferences.end();
}

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
    color_red = preferences.getUChar("ledColor_red", 128);
    color_green = preferences.getUChar("ledColor_green", 128);
    color_blue = preferences.getUChar("ledColor_blue", 128);
    preferences.end();
    userColor = CRGB(color_red, color_green, color_blue);
}

void writeLedColor() {
    preferences.begin("ledColor", false);
    preferences.putUChar("ledColor_red", color_red);
    preferences.putUChar("ledColor_green", color_green);
    preferences.putUChar("ledColor_blue", color_blue);
    preferences.end();
}

// LED Count per strip (Back, Center, Front)
void readLedCount() {
    preferences.begin("ledCount", true);
    backLeds = preferences.getInt("ledCount_back", 20);
    centerLeds = preferences.getInt("ledCount_center", 100);
    frontLeds = preferences.getInt("ledCount_front", 20);
    updateLedChannels();
    preferences.end();
}

void writeLedCount() {
    preferences.begin("ledCount", false);
    preferences.putInt("ledCount_back", backLeds);
    preferences.putInt("ledCount_center", centerLeds);
    preferences.putInt("ledCount_front", frontLeds);
    preferences.end();
}

void updateLedChannels() {
    int ledCountPerChannel = backLeds + centerLeds + frontLeds;
    ledCount[0] = ledCountPerChannel;
    ledCount[1] = ledCountPerChannel;
}

// Updates per second
void readUpdatesPerSecond() {
    preferences.begin("updatesPerSecond", true);
    updatesPerSecond = preferences.getInt("updatesPerSecond", 0);
    preferences.end();
}

void writeUpdatesPerSecond() {
    preferences.begin("updatesPerSecond", false);
    preferences.putInt("updatesPerSecond", updatesPerSecond);
    preferences.end();
}

