#include <led-control.h>
#include <PaletteEffects.h>
#include <utilities.h>

#define LED_TYPE WS2812
#define COLOR_ORDER GRB

// ---------------------------------------
// LED Related Fields
// ---------------------------------------

// Led Count per strip

int ledCount[STRIP_CHANNELS] = {149, 149};

CRGB Pixels[STRIP_CHANNELS][MAX_PIXELS_PER_STRIP];
Profiles activeProfile = solid;

int updatesPerSecond = 60;
byte brightness = 255;
byte color_red = 255;
byte color_green = 255;
byte color_blue = 255;

CRGB userColor = CRGB(255, 0, 0);

CRGBPalette16 currentPalette;
TBlendType currentBlending;

// Ripple variables
int color;
int center = 0;
int step = -1;
int maxSteps = 16;
float fadeRate = 0.80;
int diff;

// background color
uint32_t currentBg = random(256);
uint32_t nextBg = currentBg;

void initLeds()
{
    FastLED.setMaxPowerInVoltsAndMilliamps(12, 3000);

    delay(3000); // power-up safety delay

    // Channel 1
    FastLED.addLeds<LED_TYPE, CHANNEL_1_PIN, COLOR_ORDER>(Pixels[0], ledCount[0]).setCorrection(TypicalLEDStrip);
    // Channel 2
    FastLED.addLeds<LED_TYPE, CHANNEL_2_PIN, COLOR_ORDER>(Pixels[1], ledCount[1]).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(brightness);
    // Clear the Strips. We always start "off" for ...uhm...safety reasons or something... ;)
    FastLED.clear(true);
}

void ShowLedEffect()
{
    switch (activeProfile)
    {
    case solid:
        fill_solid(Pixels[0], ledCount[0], userColor);
        fill_solid(Pixels[1], ledCount[1], userColor);
        break;
    case cylon:
        TIMES_PER_SECOND(30)
        {
            // byte hue = beatsin8(32, 0, 255);
            for (int i = 0; i < STRIP_CHANNELS; i++)
            {
                fadeToBlackBy(Pixels[i], ledCount[i], 64);
                float pos = beatsin16(32, 0, ledCount[i] - 10);
                DrawPixels(i, pos, 10, userColor);
                FastLED.show(brightness);
            }
        }
        break;
    default:
        static uint8_t startIndex = 0;
        startIndex = startIndex + 1;
        FillLEDsFromPaletteColors(startIndex);
        break;
    }
    FastLED.delay(1000 / updatesPerSecond);
}

void meteorRain(CRGB ColorBackground, CRGB ColorMeteor, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay)
{
    // set background color
    fill_solid(Pixels[0], ledCount[0], ColorBackground);
    fill_solid(Pixels[1], ledCount[1], ColorBackground);

    for (int i = 0; i < (ledCount[0] * 2); i++)
    {
        // fade color to background color for all LEDs
        for (int j = 0; j < ledCount[0]; j++)
        {
            if ((!meteorRandomDecay) || (random(10) > 5))
            {
                Pixels[0][j] = fadeTowardColor(Pixels[0][j], ColorBackground, meteorTrailDecay);
                Pixels[1][j] = fadeTowardColor(Pixels[1][j], ColorBackground, meteorTrailDecay);
            }
        }

        // draw meteor
        for (int j = 0; j < meteorSize; j++)
        {
            if ((i - j < ledCount[0]) && (i - j >= 0))
            {
                Pixels[0][i - j] = ColorMeteor;
                Pixels[1][i - j] = ColorMeteor;
            }
        }

        FastLED.show();
        FastLED.delay(SpeedDelay);
    }
}

// Functions from Kriegsman example
CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount)
{
    nblendU8TowardU8(cur.red, target.red, amount);
    nblendU8TowardU8(cur.green, target.green, amount);
    nblendU8TowardU8(cur.blue, target.blue, amount);
    return cur;
}

// function used by "fadeTowardColor"
void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount)
{
    if (cur == target)
        return;

    if (cur < target)
    {
        uint8_t delta = target - cur;
        delta = scale8_video(delta, amount);
        cur += delta;
    }
    else
    {
        uint8_t delta = cur - target;
        delta = scale8_video(delta, amount);
        cur -= delta;
    }
}

void SnowSparkle(int SparkleDelay, int SpeedDelay)
{
    // set background color
    fill_solid(Pixels[0], ledCount[0], userColor);
    fill_solid(Pixels[1], ledCount[1], userColor);
    int Pixel[3] = {random(ledCount[0]), random(ledCount[1]), random(ledCount[2])};
    Pixels[0][Pixel[0]] = CRGB::White;
    Pixels[1][Pixel[1]] = CRGB::White;
    Pixels[2][Pixel[2]] = CRGB::White;
    FastLED.show();
    delay(SparkleDelay);
    Pixels[0][Pixel[0]] = userColor;
    Pixels[1][Pixel[1]] = userColor;
    Pixels[2][Pixel[2]] = userColor;
    FastLED.show();
    delay(SpeedDelay);
}

bool allPixelsAreBlank()
{
    int litCount = 0;
    for (int i = 0; i < ledCount[0]; i++)
    {
        if (Pixels[0][i] != CRGB(0, 0, 0))
        {
            litCount++;
        }
    }
    return litCount == 0;
}

void FlyingGradient()
{
    const byte fadeAmt = 64;
    const int cometSize = 5;

    static int iDirection = 1;
    static int iPos = 0;
    static bool flippedDirections = false;

    if (flippedDirections)
    {
        if (allPixelsAreBlank())
        {
            flippedDirections = false;
        }
    }
    else
    {
        iPos += iDirection;
        if (iPos == (ledCount[0] - cometSize) || iPos == 0)
        {
            flippedDirections = true;
            iDirection *= -1;
        }

        for (int i = 0; i < cometSize; i++)
        {
            Pixels[0][iPos + i] = CRGB(color_red, color_blue, color_green);
            Pixels[1][iPos + i] = CRGB(color_red, color_blue, color_green);
        }
    }

    // Fade the LEDs
    for (int j = 0; j < ledCount[0]; j++)
    {
        Pixels[0][j] = Pixels[0][j].fadeToBlackBy(fadeAmt);
        Pixels[1][j] = Pixels[0][j].fadeToBlackBy(fadeAmt);
    }
}

void FlyingGradientCh3()
{
    const byte fadeAmt = 64;
    const int cometSize = 5;

    static int iCh3Direction = 1;
    static int iCh3Pos = 0;
    static bool flippedDirections = false;

    if (flippedDirections)
    {
        if (allPixelsAreBlank())
        {
            flippedDirections = false;
        }
    }
    else
    {
        iCh3Pos += iCh3Direction;
        if (iCh3Pos == (ledCount[2] - cometSize) || iCh3Pos == 0)
        {
            flippedDirections = true;
            iCh3Direction *= -1;
        }

        for (int i = 0; i < cometSize; i++)
        {
            Pixels[2][iCh3Pos + i] = CRGB(color_red, color_blue, color_green);
        }
    }

    // Fade the LEDs
    for (int j = 0; j < ledCount[2]; j++)
    {
        Pixels[2][j] = Pixels[2][j].fadeToBlackBy(fadeAmt);
    }
}

// FractionalColor
//
// Returns a fraction of a color; abstracts the fadeToBlack out to this function in case we
// want to improve the color math or do color correction all in one location at a later date.

CRGB ColorFraction(CRGB colorIn, float fraction)
{
    fraction = min(1.0f, fraction);
    return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
}

void DrawPixels(int controller, float fPos, float count, CRGB color)
{
    // Calculate how much the first pixel will hold
    float availFirstPixel = 1.0f - (fPos - (long)(fPos));
    float amtFirstPixel = min(availFirstPixel, count);
    float remaining = min(count, FastLED[controller].size() - fPos);
    int iPos = fPos;

    // Blend (add) in the color of the first partial pixel

    if (remaining > 0.0f)
    {
        FastLED[controller].leds()[iPos++] += ColorFraction(color, amtFirstPixel);
        remaining -= amtFirstPixel;
    }

    // Now draw any full pixels in the middle

    while (remaining > 1.0f)
    {
        FastLED[controller].leds()[iPos++] += color;
        remaining--;
    }

    // Draw tail pixel, up to a single full pixel

    if (remaining > 0.0f)
    {
        FastLED[controller].leds()[iPos] += ColorFraction(color, remaining);
    }
}

void BlurEffect()
{
    for (int x = 0; x < STRIP_CHANNELS; x++)
    {
        uint8_t blurAmount = dim8_raw(beatsin8(3, 64, 192)); // A sinewave at 3 Hz with values ranging from 64 to 192.
        blur1d(Pixels[x], ledCount[x], blurAmount);          // Apply some blurring to whatever's already on the strip, which will eventually go black.

        uint8_t i = beatsin16(9, 0, ledCount[x]);
        uint8_t j = beatsin16(7, 0, ledCount[x]);
        uint8_t k = beatsin16(5, 0, ledCount[x]);

        // The color of each point shifts over time, each at a different speed.
        uint16_t ms = millis();
        Pixels[x][(i + j) / 2] = CHSV(ms / 29, 200, 255);
        Pixels[x][(j + k) / 2] = CHSV(ms / 41, 200, 255);
        Pixels[x][(k + i) / 2] = CHSV(ms / 73, 200, 255);
        Pixels[x][(k + i + j) / 3] = CHSV(ms / 53, 200, 255);

        FastLED.show();
    }
}

void RippleEffect()
{
    EVERY_N_MILLISECONDS(25)
    {
        for (int x = 0; x < STRIP_CHANNELS; x++)
        {

            /* if (currentBg == nextBg)
            {
                nextBg = random(256);
            }
            else if (nextBg > currentBg)
            {
                currentBg++;
            }
            else
            {
                currentBg--;
            } */
            for (uint16_t l = 0; l < ledCount[x]; l++)
            {
                Pixels[x][l] = CHSV(currentBg, 255, 50); // strip.setPixelColor(l, Wheel(currentBg, 0.1));
            }
            if (step == -1)
            {
                center = random(ledCount[x]);
                color = random(256);
                step = 0;
            }

            if (step == 0)
            {
                Pixels[x][center] = CHSV(color, 255, 255);
                step++;
            }
            else
            {
                if (step < maxSteps)
                {

                    Pixels[x][wrap(center + step, x)] = CHSV(color, 255, pow(fadeRate, step) * 255);
                    Pixels[x][wrap(center - step, x)] = CHSV(color, 255, pow(fadeRate, step) * 255);
                    if (step > 3)
                    {
                        Pixels[x][wrap(center + step - 3, x)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);
                        Pixels[x][wrap(center - step + 3, x)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);
                    }
                    step++;
                }
                else
                {
                    step = -1;
                }
            }

            FastLED.show();
        }
    }
}

int wrap(int step, int channel)
{
    if (step < 0)
        return ledCount[channel] + step;
    if (step > ledCount[channel] - 1)
        return step - ledCount[channel];
    return step;
}

void setPalette()
{
    switch (activeProfile)
    {
    case solid:
        break;
    case rainbow:
        currentPalette = RainbowColors_p;
        break;
    case ocean:
        currentPalette = OceanColors_p;
        break;
    case lava:
        currentPalette = LavaColors_p;
        break;
    case forest:
        currentPalette = ForestColors_p;
        break;
    case cloud:
        currentPalette = CloudColors_p;
        break;
    case randomPalette:
        SetupTotallyRandomPalette();
    default:
        break;
    }
}