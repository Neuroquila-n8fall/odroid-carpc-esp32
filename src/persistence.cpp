#include "persistence.h"
#include "led-control.h"
#include "settings.h"
// Path: src/persistence.cpp

Preferences preferences;

void readFirstBoot()
{
    preferences.begin("Settings", false);
    firstBoot = preferences.getBool("firstBoot", true);
    preferences.end();
}

void writeFirstBoot()
{
    preferences.begin("Settings", false);
    preferences.putBool("firstBoot", firstBoot);
    preferences.end();
}

// General Controller Settings

// Controller Name / BLE Adverstised Name
void readDeviceName() {
    String devName;
    preferences.begin("Settings", false);
    devName = preferences.getString("deviceName", "FX-Underglow");
    strcpy(deviceName, devName.c_str());
    preferences.end();
}

void writeDeviceName() {
    Serial.println("Writing Device Name");
    preferences.begin("Settings", false);
    preferences.putString("deviceName", deviceName);
    preferences.end();
}

// BLE Security Pin

void readSecurityPin() {
    preferences.begin("Settings", false);
    securityPin = preferences.getInt("securityPin", 0);
    preferences.end();
}

void writeSecurityPin() {
    preferences.begin("Settings", false);
    preferences.putInt("securityPin", securityPin);
    preferences.end();
}

// LED User Preferences

//Active Profile
void readLedProfile() {
    preferences.begin("Settings", false); 
    int32_t storedProfile = preferences.getInt("activeProfile", 0);
    activeProfile = static_cast<Profiles>(storedProfile);
    preferences.end();
}

void writeLedProfile() {
    preferences.begin("Settings", false);
    preferences.putInt("activeProfile", activeProfile);
    preferences.end();
}

//LED Brightness

void readLedBrightness() {
    preferences.begin("Settings", false);
    brightness = preferences.getInt("ledBrightness", 0);
}

void writeLedBrightness() {
    preferences.begin("Settings", false);
    preferences.putInt("ledBrightness", brightness);
    preferences.end();
}

//Single LED Color

void readLedColor() {
    preferences.begin("Settings", false);
    color_red = preferences.getUChar("ledColor_red", 128);
    color_green = preferences.getUChar("ledColor_green", 128);
    color_blue = preferences.getUChar("ledColor_blue", 128);
    preferences.end();
    userColor = CRGB(color_red, color_green, color_blue);
}

void writeLedColor() {
    preferences.begin("Settings", false);
    preferences.putUChar("ledColor_red", color_red);
    preferences.putUChar("ledColor_green", color_green);
    preferences.putUChar("ledColor_blue", color_blue);
    preferences.end();
}

// LED Count per strip (Back, Center, Front)
void readLedCount() {
    preferences.begin("Settings", false);
    backLeds = preferences.getInt("ledCount_back", 20);
    centerLeds = preferences.getInt("ledCount_center", 100);
    frontLeds = preferences.getInt("ledCount_front", 20);
    updateLedChannels();
    preferences.end();
}

void writeLedCount() {
    preferences.begin("Settings", false);
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
    preferences.begin("Settings", false);
    updatesPerSecond = preferences.getInt("updatesPerSec", 0);
    preferences.end();
}

void writeUpdatesPerSecond() {
    preferences.begin("Settings", false);
    preferences.putInt("updatesPerSec", updatesPerSecond);
    preferences.end();
}

