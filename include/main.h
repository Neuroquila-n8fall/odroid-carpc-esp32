#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include <FastLED.h>
#include <can_processor.h>

/*------------- Functions -------------*/

// Interaktion mit serieller Konsole
extern void readConsole();
// CAN Nachricht senden.
extern bool sendMessage(int address, byte len, const uint8_t *buf);
// Returns if any pending actions are queued
extern bool anyPendingActions();
// Perform actions triggered by Ignition Status changes
extern void onIgnitionStatusReceived(CANMessage frame);
// Perform actions triggered by the CAS Module (Car Access System)
extern void onCasCentralLockingReceived(CANMessage frame);
// Returns TRUE if all pixels are "black"
extern bool allPixelsAreBlank();
// Fades a single LED to black with every iteration. Call this repeatedly to receive the effect!
extern void fadeToBlack(int ledNo, byte fadeValue);

// PARTIALLY LOCKING! Spawns a meteor which runs along the strip with a glimmering trail.
extern void meteorRain(CRGB ColorBackground, CRGB ColorMeteor, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay);
extern void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount);
extern CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount);
// PARTIaLLY LOCKING! A random flash of white will occur on the strip. Resembles the sparkling of snow when it's hit by light.
extern void SnowSparkle(int SparkleDelay, int SpeedDelay);
// Comparable to the "Cylon" or "Knight Rider" effect. A section of LEDs with fading edges which moves along the strip
extern void FlyingGradient();
// Dedicated gradient effect for channel 3
extern void FlyingGradientCh3();
extern CRGB ColorFraction(CRGB colorIn, float fraction);
// Shortcut for direct pixel manipulation: Controller = Channel, fPos = Starting Position, count = amount of LEDs affected, color = CRGB
extern void DrawPixels(int controller, float fPos, float count, CRGB color);
// A blurry mess.
extern void BlurEffect();
// Resembles water drops hitting the surface of a water surface
extern void RippleEffect();

//Renders the selected "activeProfile"
extern void ShowLedEffect();

extern int wrap(int step, int channel);

// Amount of individual strip channels
const int STRIP_CHANNELS = 3;
// The maximum amount of leds per channel allowed. Keep it as it is.
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

// Frames per second to show on the strip(s). Keep this number between 30 and 60 for best performance vs. visual fluidity
extern int updatesPerSecond;
// The brightness level of the while strip(s). Allowed Range: 0-255
extern byte brightness;
// Red component of the user defined color
extern byte color_red;
// Green component of the user defined color
extern byte color_green;
// Blue component of the user defined color
extern byte color_blue;

// Strip Voltage
const float voltage = 5.0F;

// Selected color palette
extern CRGBPalette16 currentPalette;
// Selected blend type
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