#include <Arduino.h>
/*
  Projektbezogene Header
*/
#include <main.h>
#include <can_processor.h>
#include <ESP32CAN.h>
#include "driver/adc.h"
#include <CAN_config.h>
#include "FastLED.h"
#include "PaletteEffects.h"
#include "remotexy_cfg.h"
#include <Preferences.h>

#define FASTLED_INTERNAL

#define LED_TYPE WS2812
#define COLOR_ORDER GRB

#define TIMES_PER_SECOND(x) EVERY_N_MILLISECONDS(1000 / x)
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

bool ignitionOn = false;

// Led Count per strip
// Back: 22
// Center: 103
// Front: 24
int ledCount[STRIP_CHANNELS] = {149, 149, 60};

// Strip Channel 1
const int CHANNEL_1_PIN = 32;

// Strip Channel 2
const int CHANNEL_2_PIN = 12;

// Strip Channel 3
const int CHANNEL_3_PIN = 14;

CRGB userColor = CRGB(255, 0, 0);

CAN_device_t CAN_cfg;

//Preferences Storage
Preferences preferences;

// ---------------------------------------
// CAS Related
// ---------------------------------------

// Timeout for Fob Commands. Allow 5 seconds to execute commands via fob
int casCommandTimeOut = 5000;
// Timestamp of the last received fob command
long lastFobCommandMillis = 0L;
// Counter for the "Open" key on the fob
int openButtonCounter = 0;
// Counter for the "Close" key on the fob
int closeButtonCounter = 0;
// Flag for determine of the push-timeout has been reached.
bool casTimeoutReached = false;

// ---------------------------------------
// LED Related Fields
// ---------------------------------------

// "Frames" per second to display on the strip. 60 is a good figure.
int updatesPerSecond = 60;
byte brightness = 255;
byte color_red = 255;
byte color_green = 255;
byte color_blue = 255;
bool sendStats = false;

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

bool LedsEnabled = false;

// Profiles Enum
// Note: If adding profiles which should be available through the key fob they have to be inserted before "LAST_PROFILE".
//       Otherwise, add them after "LAST_PROFILE" but then you have to set the activeProfile explicitly.
typedef enum
{
  solid,
  cylon,
  rainbow,
  LAST_PROFILE,
} Profiles;

Profiles activeProfile = solid;

int profileCount = Profiles::LAST_PROFILE;

const char *profileNames[] =
    {
        "Solid",
        "Cylon",
        "Rainbow"
        };

void setup()
{
  delay(1000);
  Serial.begin(serialBaud);

  pinMode(LED_BUILTIN, OUTPUT);     // LED
  pinMode(PIN_DEBUG, INPUT_PULLUP); // Debug Switch Pin
  pinMode(PIN_LED_PSU_ENABLE, OUTPUT);

  //Disable the LED PSU immediately!
  //(wir wollen ja auf gar keinen Fall Ärger mit der Rennleitung haben, wa?)
  digitalWrite(PIN_LED_PSU_ENABLE, HIGH);

  RemoteXY_Init();

  // Disable ADC, save energy
  // NOTE: This will crash the ESP immediately. Probably because of the CAN interface...who knows
  // adc_power_release();
  // setCpuFrequencyMhz(80);

  //Restore Preferences
    preferences.begin("Led-Settings", false);
    color_red = preferences.getInt("color_red", 255);
    color_green = preferences.getInt("color_green", 255);
    color_blue = preferences.getInt("color_blue", 255);
    userColor = CRGB(color_red, color_green, color_blue);
    int storedProfile = preferences.getInt("activeProfile", 1);
    activeProfile = Profiles(storedProfile);

    brightness = preferences.getInt("brightness", 128);
    int storedBlending = preferences.getInt("currentBlending", 1);
    currentBlending = TBlendType(storedBlending);

    //Restore Preferences for the RemoteXY App
    //Map brightness range to slider
    RemoteXY.brightnessSlider = map(brightness, 0, 255, 0, 100);
    RemoteXY.effectSelect = activeProfile;
    RemoteXY.userColor_r = color_red;
    RemoteXY.userColor_g = color_green;
    RemoteXY.userColor_b = color_blue;

    userColor = CRGB(color_red, color_green, color_blue);

  // Setup CAN Module. There's no need to wait for init to complete. It's instantaneous.
  if (setupCan())
  {
    Serial.println("[Setup] CAN-Interface initialized.");
  };

  // We're dealing with 5 Volts an a maximum Power Output of 15A which translates to 75W
  //  The PSU used is capable of delivering 130W of power but the wires of the strip may become a little bit toasty.
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 15000);

  delay(3000); // power-up safety delay

  // Channel 1
  FastLED.addLeds<LED_TYPE, CHANNEL_1_PIN, COLOR_ORDER>(Pixels[0], ledCount[0]).setCorrection(TypicalLEDStrip);
  // Channel 2
  FastLED.addLeds<LED_TYPE, CHANNEL_2_PIN, COLOR_ORDER>(Pixels[1], ledCount[1]).setCorrection(TypicalLEDStrip);
  // Channel 3
  FastLED.addLeds<LED_TYPE, CHANNEL_3_PIN, COLOR_ORDER>(Pixels[2], ledCount[2]).setCorrection(TypicalLEDStrip);

  // Clear the strip
  FastLED.clear(true);

  // Set initial brightness
  FastLED.setBrightness(brightness);

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("[Setup] Ready.");

  lastFobCommandMillis = millis();
}

void loop()
{

  // Current timestamp
  currentMillis = millis();

  RemoteXY_Handler();

  //Read / Set variables set by the "remotexy" app as long as it's connected.
  if (RemoteXY.connect_flag == 1)
  {
    //Set Color
    if(RemoteXY.userColor_r != color_red) {
      color_red = RemoteXY.userColor_r;
      preferences.putInt("color_red", color_red);
      };
    if(RemoteXY.userColor_g != color_green) {
      color_green = RemoteXY.userColor_g;
      preferences.putInt("color_green", color_green);
      };
    if(RemoteXY.userColor_b != color_blue) {
      color_blue = RemoteXY.userColor_b;
      preferences.putInt("color_blue", color_blue);
      };

    userColor = CRGB(color_red, color_green, color_blue);

    // Sync Brightness. 
    // The slider goes from 0 to 100 whereas the actual brightness range of FastLED is 0x00 to 0xFF
    // Map those values accordingly.
    int actualBrightness = map(RemoteXY.brightnessSlider, 0, 100, 0, 255);

    if (actualBrightness != brightness)
    {
      brightness = actualBrightness;
      preferences.putInt("brightness", brightness);
    }

    //Sync profile
    if(RemoteXY.effectSelect != activeProfile)
    {
      activeProfile = Profiles(RemoteXY.effectSelect);
      preferences.putInt("activeProfile", activeProfile);
    }

    //Sync ON/OFF State.
    //As a safety measure we can't activate the lights when the ignition is on
    if (!ignitionOn)
    {
      LedsEnabled = RemoteXY.LedsEnabled == 1;
    }
    

  }
  

  if (LedsEnabled)
  {
    setPalette();
    ShowLedEffect();
  }

  // Process CAN messages
  processCanMessages();

  EVERY_N_SECONDS(1)
  {
    // Heartbeat LED
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }

    digitalWrite(LED_BUILTIN, ledState);
  }

  // Check if CAS Key Event has a timeout and reset counters if this is the case.
  if (currentMillis - lastFobCommandMillis >= casCommandTimeOut && !casTimeoutReached)
  {
    Serial.println("CAS Timeout Reached. Resetting Counters.");
    openButtonCounter = 0;
    closeButtonCounter = 0;
    casTimeoutReached = true;
  }
}

void processCanMessages()
{
  CAN_frame_t rx_frame;

  // receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {

    // Buffer in data kopieren.
    byte i = rx_frame.FIR.B.DLC;
    while (i--)
      *(frame.data + i) = *(rx_frame.data.u8 + i);

    frame.len = rx_frame.FIR.B.DLC;
    frame.rxId = rx_frame.MsgID;

    previousCanMsgTimestamp = currentMillis;
    canbusEnabled = true;

    uint32_t canId = frame.rxId;

    // Output all messages if debug is active
    if (debugCanMessages)
    {
      printCanMsgCsv(canId, frame.data, frame.len);
    }

    switch (canId)
    {

    case IDRIVE_CTRL_INIT_RESPONSE_ADDR:
    {
      Serial.println("[iDRIVE] Controller Init OK");
      iDriveInitSuccess = true;
      break;
    }
    case IDRIVE_CTRL_STATUS_ADDR:
    {
      onIdriveStatusReceived(frame);
      break;
    }

    case CAS_ZV_ADDR:
    {
      onCasCentralLockingReceived(frame);
    }

    case CAS_ADDR:
    {
      onIgnitionStatusReceived(frame);
      break;
    }

    default:
      break;
    }
  }

  // Timeout for Canbus.
  if (currentMillis - previousCanMsgTimestamp >= CAN_TIMEOUT && canbusEnabled == true)
  {
    // CAN shut down. No more messages were transmitted within the timeout period
    // The iDrive Controller is now considered uninitialized and will require the init messages again the next time it's been woken up.
    iDriveInitSuccess = false;
    canbusEnabled = false;

    Serial.println("[processCanMessages] CAN message timeout reached. Network is considered offline.");
  }
}

void printCanMsg(int canId, unsigned char *buffer, int len)
{
  // OUTPUT:
  // [ID] FF  FF  FF  FF  FF  FF  FF  FF
  Serial.print('[');
  Serial.print(canId, HEX);
  Serial.print(']');
  Serial.print('\t');
  for (int i = 0; i < len; i++)
  {
    Serial.print(buffer[i], HEX);
    Serial.print("\t");
  }
  Serial.println();
}

void printCanMsgCsv(int canId, uint8_t *buffer, int len)
{
  // OUTPUT:
  // ID;LEN;FF;FF;FF;FF;FF;FF;FF;FF
  Serial.print(canId, HEX);
  Serial.print(';');
  Serial.print(len);
  Serial.print(';');
  for (int i = 0; i < len; i++)
  {
    Serial.print(buffer[i], HEX);
    // Semikolon nicht beim letzten Eintrag anhängen
    if (i < len - 1)
    {
      Serial.print(';');
    }
  }
  Serial.println();
}

bool sendMessage(int address, byte len, const uint8_t *buf)
{
  CAN_frame_t frameToSend;
  frameToSend.MsgID = address;
  frameToSend.FIR.B.DLC = len;
  frameToSend.FIR.B.FF = CAN_frame_std;

  // Copy buffer to data
  byte i = len;
  while (i--)
    *(frameToSend.data.u8 + i) = *(buf + i);

  return ESP32Can.CANWriteFrame(&frameToSend) == 1;
}

bool setupCan()
{
  // set CAN pins and baudrate
  CAN_cfg.speed = CAN_SPEED_100KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_25;
  CAN_cfg.rx_pin_id = GPIO_NUM_26;
  // create a queue for CAN receiving
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));
  // initialize CAN Module
  int errorCode = ESP32Can.CANInit();

  previousCanMsgTimestamp = millis();

  return errorCode == 0;
}

void onIdriveStatusReceived(CANMessage frame)
{
  if (frame.data[4] == 6)
  {
    Serial.println("[iDRIVE] Controller rotation not initialized.");
    // Controller told us that it hasn't been initialized. Send the message now to do just that:
    sendMessage(IDRIVE_CTRL_INIT_ADDR, 8, IDRIVE_CTRL_INIT);
    iDriveInitSuccess = false;
  }
  else
  {
    iDriveInitSuccess = true;
  }
}

void onIgnitionStatusReceived(CANMessage frame)
{
  if (frame.data[0] == 0x45 || frame.data[0] == 0x55)
  {
    if (!ignitionOn)
    {
      FastLED.clear(true);
      LedsEnabled = false;
      digitalWrite(PIN_LED_PSU_ENABLE, HIGH);
      Serial.println("Ignition or Engine is now ON");
    }

    ignitionOn = true;

    return;
  }

  if(frame.data[0] == 0x00 || frame.data[0] == 0x40 || frame.data[0] == 0x41)
  {
    
    if (ignitionOn)
    {
      ignitionOn = false;
      Serial.println("Ignition or Engine is now OFF");
    }
  }
}

void onCasCentralLockingReceived(CANMessage frame)
{
  // Debounce: Befehle werden erst wieder verarbeitet, wenn der Timeout abgelaufen ist.
  if (currentMillis - previousCasMessageTimestamp > CAS_DEBOUNCE_TIMEOUT)
  {
    previousCasMessageTimestamp = currentMillis;
    casTimeoutReached = false;
    //Öffnen:     00CF01FF
    if (frame.data[0] == 0x00 && frame.data[1] == 0x30 && frame.data[2] == 0x01 && frame.data[3] == 0x60)
    {
      lastFobCommandMillis = currentMillis;
      openButtonCounter++;
      Serial.print("Open Button Counter: ");
      Serial.println(openButtonCounter);
      if (openButtonCounter == 3 && !LedsEnabled)
      {
        // Start the LED Show!
        LedsEnabled = true;
        digitalWrite(PIN_LED_PSU_ENABLE, LOW);
        openButtonCounter = 0;
        Serial.println("LEDs enabled by Keyfob command.");
      }
      if (openButtonCounter == 1 && LedsEnabled)
      {
        // Switch to next effect
        if (activeProfile < profileCount - 1)
        {
          activeProfile = Profiles(activeProfile + 1);
          openButtonCounter = 0;
          Serial.print("Switching to profile: ");
          Serial.println(profileNames[activeProfile]);
        }
        else
        {
          activeProfile = Profiles(0);
          Serial.print("End of Profiles. Switching to profile: ");
          Serial.println(profileNames[activeProfile]);
          FastLED.clear(true);
        }
      }
    }
    // Schließen:  00DF40FF
    if (frame.data[0] == 0x00 && frame.data[1] == 0x30 && frame.data[2] == 0x04 && frame.data[3] == 0x60)
    {
      lastFobCommandMillis = currentMillis;
      // LEDs ausschalten.
      FastLED.clear(true);
      LedsEnabled = false;
      activeProfile = Profiles(0);
      Serial.println("Car Closed. LEDs disabled.");
    }
    // Kofferraum: Wird nur gesendet bei langem Druck auf die Taste
  }
}

void ShowLedEffect()
{
  switch (activeProfile)
  {
  case solid:
  {
    fill_solid(Pixels[0], ledCount[0], userColor);
    fill_solid(Pixels[1], ledCount[1], userColor);
    fill_solid(Pixels[2], ledCount[2], userColor);
    break;
  }
  case cylon:
  {
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
  }
  default:
  {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1;
    FillLEDsFromPaletteColors(startIndex);
    break;
  }
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

// Fades the current color to the target color by specified amount
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
  fill_solid(Pixels[2], ledCount[2], userColor);
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
      // The following commented block is kept for later use. It might be useful for anyone who whishes to swap in a random background
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

// Wraps Pixels around 
int wrap(int step, int channel)
{
  if (step < 0)
    return ledCount[channel] + step;
  if (step > ledCount[channel] - 1)
    return step - ledCount[channel];
  return step;
}

// Switches the color palette according to the selected profile.
void setPalette()
{
  switch (activeProfile)
  {
  case solid:
    break;
  case rainbow:
    currentPalette = RainbowColors_p;
    break;
  /* case ocean:
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
      SetupTotallyRandomPalette(); */
  default:
    break;
  }
}