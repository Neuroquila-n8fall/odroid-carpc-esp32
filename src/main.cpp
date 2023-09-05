#include <Arduino.h>
/*
  Projektbezogene Header
*/
#include <main.h>

#include <ESP32CAN.h>
#include <CAN_config.h>
#include "FastLED.h"
#include <PaletteEffects.h>
#include "persistence.h"
#include "pins.h"

#include <k-can-addresses.h>
#include <k-can-messages.h>

#include <ble_controller.h>
#include <settings.h>
#include <led-control.h>

#include <nvs_flash.h>

bool ignitionOn = false;

CAN_device_t CAN_cfg;

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

bool run = false;
bool justBooted = true;

ulong fiveSecondTimer;
ulong delayTimer;

bool LedsEnabled = false;

BLEController bleController;

unsigned long currentMillis = 0L;

bool configurationResetRequested = false;

void setup()
{
  delay(1000);
  Serial.begin(serialBaud);

  pinMode(LED_BUILTIN, OUTPUT);     // LED
  pinMode(PIN_DEBUG, INPUT_PULLUP); // Debug Switch Pin

  #pragma region Startup Interrupt

  unsigned long curmils = millis();

  while (millis() - curmils <= 3000)
  {

    // Blink LED to indicate reset time window
    EVERY_N_MILLISECONDS(500)
    {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    //Check GPIO0 for presses which triggers the configuration reset process
    configurationResetRequested = !digitalRead(GPIO_NUM_0);

    if (configurationResetRequested)
    {
      break;
    }
    
  }

  //Perform Reset Actions
  if (configurationResetRequested)
  {
    Serial.println("[Setup] Configuration Reset Requested. Resetting Configuration...");

    // Erase the NVS partition
    nvs_flash_erase();

    // Initialize the NVS partition again
    nvs_flash_init();

    Serial.println("[Setup] Configuration Reset Complete. Rebooting...");

    // Reset the ESP32
    esp_restart();
  }
  
  

  #pragma endregion

  // Read LED Settings from NVRAM
  readLedProfile();

  // Read LED Brightness from NVRAM
  readLedBrightness();

  // Read LED Color from NVRAM
  readLedColor();

  // Setup CAN Module. Es ist nicht notwendig auf den Bus zu warten
  if (setupCan())
  {
    Serial.println("[Setup] CAN-Interface initialized.");
  };

  digitalWrite(LED_BUILTIN, HIGH);

  // Setup LED Strip
  initLeds();

  Serial.println("[Setup] Ready.");

  // Read Device Name from NVRAM
  readDeviceName();
  // Read Security Pin from NVRAM
  readSecurityPin();

  // Start BLE Controller
  bleController.init(deviceName, securityPin);
}

void loop()
{
  if (LedsEnabled)
  {
    setPalette();
    ShowLedEffect();
  }

  // CAN Nachrichten verarbeiten
  processCanMessages();
  EVERY_N_SECONDS(1)
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
  }

  // If a device was connected but has now disconnected, restart advertising
  if (bleController.isDeviceConnected() && !bleController.wasDeviceConnected())
  {
    delay(500); // Give it a short delay
    BLEDevice::startAdvertising();
  }
  bleController.setOldDeviceConnected(bleController.isDeviceConnected());
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

void onCasCentralLockingReceived(CANMessage frame)
{
  // Debounce: Befehle werden erst wieder verarbeitet, wenn der Timeout abgelaufen ist.
  if (currentMillis - previousCasMessageTimestamp > CAS_DEBOUNCE_TIMEOUT)
  {
    previousCasMessageTimestamp = currentMillis;

    if (lastFobCommandMillis - currentMillis >= casCommandTimeOut)
    {
      openButtonCounter = 0;
      closeButtonCounter = 0;
    }
    // Öffnen:     00CF01FF
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
      Serial.println("Car Closed. LEDs disabled.");
    }
    // Kofferraum: Wird nur gesendet bei langem Druck auf die Taste
  }
}