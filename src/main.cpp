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

// ------------------------------------
// Indicator Variables and Fields
// ------------------------------------

// Timestamp for last indicator "tick"
long IndicatorMillis = 0L;
// Enum for Indicators
typedef enum
{
  Left,
  Right,
  Hazard,
  Off
} Indicator;

// The currently active Indicator or mode.
Indicator ActiveIndicator = Off;
// Color of the indicator.
CRGB IndicatorColor = CRGB(252, 165, 3);

// ---------------------------------------
// CAS Related
// ---------------------------------------
// Timeout for Fob Commands. Allow 5 seconds to execute commands via fob
int casCommandTimeOut = 5000;
long lastFobCommandMillis = 0L;
int openButtonCounter = 0;
int closeButtonCounter = 0;

// ---------------------------------------
// Door Status
// ---------------------------------------
bool driverDoorOpen = false;
bool passengerDoorOpen = false;

// ---------------------------------------
// LED Related Fields
// ---------------------------------------

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

bool run = false;
bool justBooted = true;

ulong fiveSecondTimer;
ulong delayTimer;

bool LedsEnabled = false;

// Profiles Enum
typedef enum
{
  car,
  solid,
  cylon,
  rainbow,
  meteor,
  snowSparkle,
  blurry,
  ripple,
} Profiles;

Profiles activeProfile = solid;

const int profileCount = 8;
Profiles profileList[profileCount] =
    {
        car,
        solid,
        cylon,
        rainbow,
        meteor,
        snowSparkle,
        blurry,
        ripple};

void setup()
{
  delay(1000);
  Serial.begin(serialBaud);

  pinMode(LED_BUILTIN, OUTPUT);     // LED
  pinMode(PIN_DEBUG, INPUT_PULLUP); // Debug Switch Pin

  hibernateActive = false;

  // Disable ADC, save energy
  // adc_power_release();
  // setCpuFrequencyMhz(80);

  // Setup CAN Module. Es ist nicht notwendig auf den Bus zu warten
  if (setupCan())
  {
    Serial.println("[Setup] CAN-Interface initialized.");
  };

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);

  delay(3000); // power-up safety delay

  // Channel 1
  FastLED.addLeds<LED_TYPE, CHANNEL_1_PIN, COLOR_ORDER>(Pixels[0], ledCount[0]).setCorrection(TypicalLEDStrip);
  // Channel 2
  FastLED.addLeds<LED_TYPE, CHANNEL_2_PIN, COLOR_ORDER>(Pixels[1], ledCount[1]).setCorrection(TypicalLEDStrip);
  // Channel 3
  FastLED.addLeds<LED_TYPE, CHANNEL_3_PIN, COLOR_ORDER>(Pixels[2], ledCount[2]).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("[Setup] Ready.");
}

void loop()
{

  // Aktuelle Zeit
  currentMillis = millis();

  // Indicator lights are only on for half a second.
  if (currentMillis - IndicatorMillis >= 500 && ActiveIndicator != Off)
  {
    ActiveIndicator = Off;
    showIndicator();
  }

  if (LedsEnabled)
  {
    setPalette();
    ShowLedEffect();
  }

  // CAN Nachrichten verarbeiten
  processCanMessages();

  // 1-Second timer
  if (currentMillis - previousOneSecondTick >= 1000)
  {
    // Heartbeat
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }

    digitalWrite(LED_BUILTIN, ledState);
    previousOneSecondTick = currentMillis;
  }

  // Check if CAS Key Event has a timeout and reset counters if this is the case.
  if (currentMillis - lastFobCommandMillis >= casCommandTimeOut)
    {
      Serial.println("CAS Timeout Reached. Resetting Counters.");
      openButtonCounter = 0;
      closeButtonCounter = 0;
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

    // Normalbetrieb
    if (hibernateActive)
    {
      // exitPowerSaving();
    }

    uint32_t canId = frame.rxId;

    // Alle CAN Nachrichten ausgeben, wenn debug aktiv.
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

    case JBE_DRIVER_DOOR_STATUS_ADDR:
    {

      onDriverDoorStatusReceived(frame);
      break;
    }

    case JBE_PASSENGER_DOOR_STATUS_ADDR:
    {

      onPassengerDoorStatusReceived(frame);
      break;
    }

    case FRM_INDICATOR_ADDR:
    {
      onIndicatorStatusReceived(frame);
      break;
    }

    case INDICATOR_STALK_ADDR:
    {
      onIndicatorStalkReceived(frame);
      break;
    }

    case CAS_ZV_ADDR:
    {
      onCasCentralLockingReceived(frame);
    }

    default:
      break;
    }
  }

  // Timeout für Canbus.
  if (currentMillis - previousCanMsgTimestamp >= CAN_TIMEOUT && canbusEnabled == true)
  {
    // Canbus wurde heruntergefahren. Es werden keinerlei Nachrichten mehr seit 30 Sekunden ausgetauscht.
    // Der iDrive Controller ist jetzt als deaktiviert zu betrachten und muss neu intialisiert werden
    iDriveInitSuccess = false;
    canbusEnabled = false;

    Serial.println("[checkCan] Keine Nachrichten seit 30 Sekunden. Der Bus wird nun als deaktiviert betrachtet.");
    // enterPowerSaving();
  }
}

void enterPowerSaving()
{
  Serial.println("[Power] Entering power saving...");
  setCpuFrequencyMhz(80);
  hibernateActive = true;
}

void exitPowerSaving()
{
  Serial.println("[Power] Exiting power saving mode...");
  setCpuFrequencyMhz(240);
  hibernateActive = false;
}

void printCanMsg(int canId, unsigned char *buffer, int len)
{
  // OUTPUT:
  // ABC FF  FF  FF  FF  FF  FF  FF  FF
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
  // ABC FF  FF  FF  FF  FF  FF  FF  FF
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

  // Buffer in data kopieren.
  byte i = len;
  while (i--)
    *(frameToSend.data.u8 + i) = *(buf + i);

  return ESP32Can.CANWriteFrame(&frameToSend) == 1;
}

bool setupCan()
{
  /* set CAN pins and baudrate */
  CAN_cfg.speed = CAN_SPEED_100KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_25;
  CAN_cfg.rx_pin_id = GPIO_NUM_26;
  /* create a queue for CAN receiving */
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
    Serial.println("[iDRIVE] Controller ist nicht initialisiert.");
    // Controller meldet er sei nicht initialisiert: Nachricht zum Initialisieren senden.
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
  // OFF: 00 00 3F
  // ON: 064 064 127
  if (frame.data[0] == 0x64 && frame.data[1] == 0x64 && frame.data[2] == 0x127)
  {
    ignitionOn = true;
    FastLED.clear(true);
    Serial.println("Ignition is ON");
    return;
  }

  ignitionOn = false;
  Serial.println("Ignition is OFF");
}

// Messages are sent every second. Apparently the "Off" Message is for all indicators but has to be tested first.
void onIndicatorStatusReceived(CANMessage frame)
{
  // Initial, ON, [...], OFF
  // Indicators are set to OFF every 500ms after they have been triggered ON
  // OFF Signal
  if (frame.data[0] == 0x80)
  {
    // Indicators OFF
    Serial.println("Indicators OFF");
    ActiveIndicator = Off;
  }

  // LEFT Turn Signal
  if (frame.data[0] == 0x91)
  {
    IndicatorMillis = millis();
    ActiveIndicator = Left;
    // Initial Message
    if (frame.data[1] == 0xF2)
    {
      // Initial Indicator Message
      Serial.println("LEFT Indicator INITIAL");
    }
    // Message followed by 0xF2
    if (frame.data[1] == 0xF1)
    {
      // Indicator Message
      Serial.println("LEFT Indicator ON");
    }
  }

  // RIGHT Turn Signal
  if (frame.data[0] == 0xA1)
  {
    IndicatorMillis = millis();
    ActiveIndicator = Right;
    // Initial Message
    if (frame.data[1] == 0xF2)
    {
      // Initial Indicator Message
      Serial.println("RIGHT Indicator INITIAL");
    }
    // Message followed by 0xF2
    if (frame.data[1] == 0xF1)
    {
      // Indicator Message
      Serial.println("RIGHT Indicator ON");
    }
  }

  // HAZARD Signal
  if (frame.data[0] == 0xB1)
  {
    IndicatorMillis = millis();
    ActiveIndicator = Hazard;
    // Initial Message
    if (frame.data[1] == 0xF2)
    {
      // Initial Indicator Message
      Serial.println("HAZARD Indicator INITIAL");
    }
    // Message followed by 0xF2
    if (frame.data[1] == 0xF1)
    {
      // Indicator Message
      Serial.println("HAZARD Indicator ON");
    }
  }

  showIndicator();
}

void showIndicator()
{
  // Do nothing if not explicitly enabled!
  if (!LedsEnabled)
    return;

  switch (ActiveIndicator)
  {
  case Left:
  {
    fill_solid(Pixels[0], ledCount[0], IndicatorColor);
    break;
  }
  case Right:
  {
    fill_solid(Pixels[1], ledCount[1], IndicatorColor);
    break;
  }
  case Hazard:
  {
    fill_solid(Pixels[0], ledCount[0], IndicatorColor);
    fill_solid(Pixels[1], ledCount[1], IndicatorColor);
    break;
  }
  case Off:
  {
    FastLED.clear(true);
    break;
  }

  default:
    break;
  }
}

void onIndicatorStalkReceived(CANMessage frame)
{

  switch (frame.data[0])
  {
    // Indicator UP -> RIGHT Short
  case 0x01:
  {
    break;
  }
    // Indicator UP -> RIGHT Permanent
  case 0x02:
  {
    break;
  }
    // Indicator DOWN -> LEFT Short
  case 0x03:
  {
    break;
  }
    // Indicator Down -> Left Permanent
  case 0x04:
  {
    break;
  }

  default:
    break;
  }
}

void onDriverDoorStatusReceived(CANMessage frame)
{
  if (frame.data[3] == 0xFD)
  {
    Serial.println("Driver Door CLOSED");
    driverDoorOpen = false;
  }
  if (frame.data[3] == 0xFC)
  {
    Serial.println("Driver Door OPEN");
    driverDoorOpen = true;
  }
}

void onPassengerDoorStatusReceived(CANMessage frame)
{
  if (frame.data[3] == 0xFD)
  {
    Serial.println("Passenger Door CLOSED");
    passengerDoorOpen = false;
  }
  if (frame.data[3] == 0xFC)
  {
    Serial.println("Passenger Door OPEN");
    passengerDoorOpen = true;
  }
}

void onCasCentralLockingReceived(CANMessage frame)
{
  // Debounce: Befehle werden erst wieder verarbeitet, wenn der Timeout abgelaufen ist.
  if (currentMillis - previousCasMessageTimestamp > CAS_DEBOUNCE_TIMEOUT)
  {
    previousCasMessageTimestamp = currentMillis;

    
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
        openButtonCounter = 0;
        Serial.println("LEDs enabled by Keyfob command.");
      }
      if (openButtonCounter == 1 && LedsEnabled)
      {
        // Switch to next effect
        if (activeProfile < profileCount)
        {
          // Rather hacky way to increment an enum
          activeProfile = profileList[activeProfile + 1];
          openButtonCounter = 0;
          Serial.print("Switching to profile: ");
          Serial.println(activeProfile);
        }
        else
        {
          activeProfile = profileList[0];
          Serial.print("End of Profiles. Switching to profile: ");
          Serial.println(activeProfile);
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
      activeProfile = car;
      Serial.println("Car Closed. LEDs disabled.");
    }
    // Kofferraum: Wird nur gesendet bei langem Druck auf die Taste
  }
}

void ShowLedEffect()
{
  switch (activeProfile)
  {
  case car:
    // Do Car Lighting Related Stuff.
    if (driverDoorOpen)
    {
      for (int i = BackLeds + 1; i < CenterLeds; i++)
      {
        Pixels[0][i] = CRGB::White;
      }
    }

    if (passengerDoorOpen)
    {
      for (int i = BackLeds + 1; i < CenterLeds; i++)
      {
        Pixels[1][i] = CRGB::White;
      }
    }

    // Turn off lights when the door is closed and the indicator is off.
    if (!driverDoorOpen && ActiveIndicator == Off)
    {
      for (int i = BackLeds + 1; i < CenterLeds; i++)
      {
        Pixels[0][i] = CRGB::Black;
      }
    }

    // Turn off lights when the door is closed and the indicator is off.
    if (!passengerDoorOpen && ActiveIndicator == Off)
    {
      for (int i = BackLeds + 1; i < CenterLeds; i++)
      {
        Pixels[1][i] = CRGB::Black;
      }
    }
    FastLED.show(brightness);
    break;
  case solid:
    fill_solid(Pixels[0], ledCount[0], userColor);
    fill_solid(Pixels[1], ledCount[1], userColor);
    fill_solid(Pixels[2], ledCount[2], userColor);
    break;
  case meteor:
    if (millis() - delayTimer >= random(5000, 20000))
    {
      delayTimer = millis();
      meteorRain(CRGB(0, 0, 0), userColor, 10, 64, true, 0);
    }

    break;
  case snowSparkle:
    SnowSparkle(30, random(100, 1000));
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
  case blurry:
    BlurEffect();
    break;
  case ripple:
    TIMES_PER_SECOND(updatesPerSecond)
    {
      RippleEffect();
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