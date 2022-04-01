#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include <FastLED.h>
#include <can_processor.h>

/*------------- Functions -------------*/

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

extern void onIgnitionStatusReceived(CANMessage frame);
extern void onIndicatorStatusReceived(CANMessage frame);
extern void onIndicatorStalkReceived(CANMessage frame);
extern void onDriverDoorStatusReceived(CANMessage frame);
extern void onPassengerDoorStatusReceived(CANMessage frame);
extern void showIndicator();
extern void onCasCentralLockingReceived(CANMessage frame);
 
extern bool allPixelsAreBlank();

extern void fadeToBlack(int ledNo, byte fadeValue);
extern void meteorRain(CRGB ColorBackground, CRGB ColorMeteor, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay);
extern void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount);
extern CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount);
extern void SnowSparkle(int SparkleDelay, int SpeedDelay);
extern void FlyingGradient();
extern void FlyingGradientCh3();
extern CRGB ColorFraction(CRGB colorIn, float fraction);
extern void DrawPixels(int controller, float fPos, float count, CRGB color);
extern void BlurEffect();
extern void RippleEffect();
extern void ShowLedEffect();

extern int wrap(int step, int channel);

const int STRIP_CHANNELS = 3;
const int MAX_PIXELS_PER_STRIP = 2048;

// Pixels
CRGB Pixels[STRIP_CHANNELS][MAX_PIXELS_PER_STRIP];

// Led Count per strip
// Back: 22
// Center: 103
// Front: 24
extern int ledCount[STRIP_CHANNELS];
const int BackLeds = 22;
const int CenterLeds = 103;
const int FrontLeds = 24;


extern int updatesPerSecond;
extern byte brightness;
extern byte color_red;
extern byte color_green;
extern byte color_blue;
extern bool sendStats;

const float voltage = 5.0F;

extern CRGBPalette16 currentPalette;
extern TBlendType currentBlending;

//Ripple variables
extern int color;
extern int center;
extern int step;
extern int maxSteps;
extern float fadeRate;
extern int diff;
//background color
extern uint32_t currentBg;
extern uint32_t nextBg;

#endif