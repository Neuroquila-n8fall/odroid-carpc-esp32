#ifndef _LEDCONTROL_H
#define _LEDCONTROL_H

#include <Arduino.h>
#include <FastLED.h>

// Strip Channel 1
const int CHANNEL_1_PIN = 25;

// Strip Channel 2
const int CHANNEL_2_PIN = 26;

// Strip Channel 3
const int CHANNEL_3_PIN = 14;

const int STRIP_CHANNELS = 2;
const int MAX_PIXELS_PER_STRIP = 2048;

// Pixels
extern CRGB Pixels[STRIP_CHANNELS][MAX_PIXELS_PER_STRIP];

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

extern CRGBPalette16 currentPalette;
extern TBlendType currentBlending;

// Profiles Enum
typedef enum
{
    solid,
    cylon,
    rainbow,
    ocean,
    fire,
    forest,
    lava,
    cloud,
    randomPalette
} Profiles;

const int profileCount = 9;
const Profiles profileList[profileCount] =
    {
        solid,
        cylon,
        rainbow,
        ocean,
        fire,
        forest,
        lava,
        cloud,
        randomPalette};


// Profile
extern Profiles activeProfile;

// Ripple variables
extern int color;
extern int center;
extern int step;
extern int maxSteps;
extern float fadeRate;
extern int diff;


// background color
extern uint32_t currentBg;
extern uint32_t nextBg;


// --- Functions

extern void initLeds();

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

#endif