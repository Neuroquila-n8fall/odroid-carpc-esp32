#ifndef _PALETTES_H
#define _PALETTES_H

#include <Arduino.h>
#include <FastLED.h>

#define FASTLED_INTERNAL

extern CRGBPalette16 myRedWhiteBluePalette;

void FillLEDsFromPaletteColors(uint8_t colorIndex);
void ChangePalettePeriodically();
void SetupBlackAndWhiteStripedPalette();
void SetupTotallyRandomPalette();
void SetupPurpleAndGreenPalette();
void setPalette();

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
    {
        CRGB::Red,
        CRGB::Gray, // 'white' is too bright compared to red and blue
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Red,
        CRGB::Gray,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Blue,
        CRGB::Black,
        CRGB::Black};

#endif