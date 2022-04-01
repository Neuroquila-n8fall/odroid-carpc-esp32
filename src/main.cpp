#include <Arduino.h>
/*
  Projektbezogene Header
*/
#include <main.h>
#include <can_processor.h>
#include <ESP32CAN.h>
#include "driver/adc.h"
#include <CAN_config.h>

CAN_device_t CAN_cfg;

void setup()
{
  delay(1000);
  Serial.begin(serialBaud);

  pinMode(LED_BUILTIN, OUTPUT);                     //LED
  pinMode(PIN_DEBUG, INPUT_PULLUP);                 // Debug Switch Pin

  hibernateActive = false;

  //Disable ADC, save energy  
  //adc_power_release();
  //setCpuFrequencyMhz(80);

  //Setup CAN Module. Es ist nicht notwendig auf den Bus zu warten
  if(setupCan())
  {
    Serial.println("[Setup] CAN-Interface initialized.");
  };

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("[Setup] Ready.");
}

void loop()
{

  //Aktuelle Zeit
  currentMillis = millis();

  //CAN Nachrichten verarbeiten
  processCanMessages();

  //1-Second timer
  if (currentMillis - previousOneSecondTick >= 1000)
  {
    //Heartbeat
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

    //Alle CAN Nachrichten ausgeben, wenn debug aktiv.
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
    
    default:
      break;
    }
  }

  //Timeout für Canbus.
  if (currentMillis - previousCanMsgTimestamp >= CAN_TIMEOUT && canbusEnabled == true)
  {
    //Canbus wurde heruntergefahren. Es werden keinerlei Nachrichten mehr seit 30 Sekunden ausgetauscht.
    //Der iDrive Controller ist jetzt als deaktiviert zu betrachten und muss neu intialisiert werden
    iDriveInitSuccess = false;
    canbusEnabled = false;

    Serial.println("[checkCan] Keine Nachrichten seit 30 Sekunden. Der Bus wird nun als deaktiviert betrachtet.");
    //enterPowerSaving();
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
  //OUTPUT:
  //ABC FF  FF  FF  FF  FF  FF  FF  FF
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
  //OUTPUT:
  //ABC FF  FF  FF  FF  FF  FF  FF  FF
  Serial.print(canId, HEX);
  Serial.print(';');
  Serial.print(len);
  Serial.print(';');
  for (int i = 0; i < len; i++)
  {
    Serial.print(buffer[i], HEX);
    //Semikolon nicht beim letzten Eintrag anhängen
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

  //Buffer in data kopieren.
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
    //Controller meldet er sei nicht initialisiert: Nachricht zum Initialisieren senden.
    sendMessage(IDRIVE_CTRL_INIT_ADDR, 8, IDRIVE_CTRL_INIT);
    iDriveInitSuccess = false;
  }
  else
  {
    iDriveInitSuccess = true;
  }
}