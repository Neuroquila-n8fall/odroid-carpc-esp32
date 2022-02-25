#include <Arduino.h>
#include <SPI.h>
#include <BleKeyboard.h>
#include <ACAN2515.h>
#include <analogWrite.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
/*
  Projektbezogene Header
*/
#include <main.h>
#include <can_processor.h>
#include <ble_keyboard.h>

Adafruit_INA219 ina219;
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
void setup()
{
  delay(1000);
  Serial.begin(serialBaud);
  //Zeitstempel einrichten
  buildtimeStamp();

  pinMode(PIN_IGNITION_INPUT, INPUT_PULLUP);        //Zündungs-Pin
  pinMode(PIN_ODROID_POWER_BUTTON, OUTPUT);         //Opto 2 - Odroid Power Button
  pinMode(PIN_ODROID_POWER_INPUT, INPUT_PULLUP);    //Odroid VOUT Pin als Rückmeldung ob der PC eingeschaltet ist
  pinMode(PIN_ODROID_DISPLAY_POWER_BUTTON, OUTPUT); //Opto 3 - Display Power Button
  pinMode(LED_BUILTIN, OUTPUT);                     //LED
  pinMode(PIN_DEBUG, INPUT_PULLUP);                 //Debug Switch Pin
  pinMode(PIN_VU7A_BRIGHTNESS, OUTPUT);             //Display Helligkeitssteuerung
  analogWriteResolution(PIN_VU7A_BRIGHTNESS, 12);   //DUE arbeitet mit 12 bits. Das hat immer sehr gut funktioniert.
  analogWriteFrequency(PIN_VU7A_BRIGHTNESS, 980);   //4103 LED Driver arbeitet mit bis zu 1kHz. Wir nehmen hier das was der DUE bereits geleistet hat.

  hibernateActive = false;

  //Setup CAN Module. Es ist nicht notwendig auf den Bus zu warten
  if(setupCan())
  {
    // Todo?
  };

  //Display von ganz dunkel nach ganz Hell stellen. Quasi als Test
  for (int i = 0; i <= 255; i++)
  {
    analogWrite(PIN_VU7A_BRIGHTNESS, i);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  //50ms delay zwischen Tastenanschlägen
  bleKeyboard.setDelay(50);

  // Bluetooth Modul
  bleKeyboard.begin();

  if (!ina219.begin())
  {
    Serial.println("[Setup] Failed to find INA219 chip");
  }
  else
  {
    Serial.println("[Setup] INA219 Initialized.");
  }

  // Prime voltage reading average array.
  for (int i = 0; i < maxAverageReadings; i++)
  {
    voltageReadings[i] = 12.5F;
  }

  Serial.println("[Setup] Ready.");
}

void loop()
{

  //Aktuelle Zeit
  currentMillis = millis();

  if (startup)
  {
    //Wenn die Steuerung mit aktiver Zündung startet oder resettet, sollte der Odroid starten
    //Aber nur wenn der Odroid AUS ist aber die Zündung an
    //Wenn der Odroid im Sleep ist, ist odroidRunning ebenfalls Low. Auf diese Art wird er bei Zündung direkt aufgeweckt.
    if (odroidRunning == LOW && ignitionOn == LOW && odroidStartRequested == false)
    {
      Serial.println("[STARTUP] PC aus, Zündung an --> Starte Pc...");
      startOdroid();
    }
    //Start beendet.
    startup = false;
  }

  //Start angefordert
  if (odroidStartRequested)
  {
    if (currentMillis - previousOdroidActionTime >= ODROID_BOOT_HOLD_DELAY)
    {
      odroidStartRequested = false;
      //Zeit merken
      previousOdroidActionTime = currentMillis;
      //Ausgang freigeben
      digitalWrite(PIN_ODROID_POWER_BUTTON, LOW);
      Serial.println("[odroidStartRequested] Start erfolgt.");
      //Aktuellen Counter merken. Ausführung weiterer Aktionen wird pausiert, bis abgewartet wurde.
      startPowerTransitionMillis = millis();
    }
  }
  //Stop angefordert
  if (odroidShutdownRequested)
  {
    if (currentMillis - previousOdroidActionTime >= ODROID_SHUTDOWN_HOLD_DELAY)
    {
      odroidShutdownRequested = false;
      //Zeit merken
      previousOdroidActionTime = currentMillis;
      //Ausgang freigeben
      digitalWrite(PIN_ODROID_POWER_BUTTON, LOW);
      Serial.println("[odroidShutdownRequested] Stop erfolgt.");
      //Aktuellen Counter merken. Ausführung weiterer Aktionen wird pausiert, bis abgewartet wurde.
      startPowerTransitionMillis = millis();
    }
  }

  if (odroidPauseRequested)
  {
    if (currentMillis - previousOdroidPauseTime >= ODROID_STANDBY_DELAY)
    {
      odroidPauseRequested = false;
      Serial.println("[odroidPauseRequested] Standby erfolgt.");
      //Aktuellen Counter merken. Ausführung weiterer Aktionen wird pausiert, bis abgewartet wurde.
      startPowerTransitionMillis = millis();
    }
  }

  //CAN Nachrichten verarbeiten
  processCanMessages();
  //Konsole
  readConsole();

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

    //Spannung prüfen
    checkVoltage();

    digitalWrite(LED_BUILTIN, ledState);
    previousOneSecondTick = currentMillis;
    //Aktualisieren.
    timeKeeper();
    //Zeitstempel Variablen füllen
    buildtimeStamp();
  }

  

  //Allgemeine Funktionen. Nur ausführen, wenn Zyklus erreicht wurde und keine ausstehenden Aktionen laufen, die ein zeitkritisches Verändern der Ausgänge beinhalten.
  if (currentMillis - previousMainTaskTime >= CYCLE_DELAY && !anyPendingActions())
  {
    //Zündung überprüfen
    checkIgnitionState();
    //Ein- und Ausgänge überprüfen
    checkPins();

    //Zeit merken
    previousMainTaskTime = currentMillis;
    //Sperrung freigeben, wenn Timeout abgelaufen ist
    switch (pendingAction)
    {
    case ODROID_STOP:
      if (currentMillis - startPowerTransitionMillis >= SHUTDOWN_WAIT_DELAY)
      {
        Serial.println("[LOOP] Shutdown Wartezeit abgelaufen");
        pendingAction = NONE;
        //Wurde der Start vorgemerkt, ausführen und zurücksetzen
        if (queuedAction == ODROID_START)
        {
          startOdroid();
          queuedAction = NONE;
        }
      }
      break;
    case ODROID_START:
      if (currentMillis - startPowerTransitionMillis >= STARTUP_WAIT_DELAY)
      {
        Serial.println("[LOOP] Start Wartezeit abgelaufen");
        pendingAction = NONE;
        //Wurde Stopp vorgemerkt, ausführen und zurücksetzen
        if (queuedAction == ODROID_STOP)
        {
          stopOdroid();
          queuedAction = NONE;
        }
      }
      break;
    case ODROID_STANDBY:
      if (currentMillis - startPowerTransitionMillis >= ODROID_STANDBY_DELAY)
      {
        Serial.println("[LOOP] Stand-by Wartezeit abgelaufen");
        pendingAction = NONE;
      }
      break;

    default:
      //Keine Aktion aktiv.

      //Sicherheitshalber zurücksetzen
      queuedAction = NONE;
      break;
    }
  }
}

void processCanMessages()
{
  //can.poll();
  CANMessage frame;
  if (can.available())
  {
    can.receive(frame);
    previousCanMsgTimestamp = currentMillis;
    canbusEnabled = true;

    // Normalbetrieb
    if (hibernateActive)
    {
      exitPowerSaving();
    }
    

    uint32_t canId = frame.id;

    //Alle CAN Nachrichten ausgeben, wenn debug aktiv.
    if (true)
    {
      printCanMsgCsv(canId, frame.data, frame.len);
    }

    switch (canId)
    {
    //MFL Knöpfe
    case MFL_BUTTON_ADDR:
    {
      onMflButtonPressed(frame);
      break;
    }
    //CAS: Schlüssel & Zündung
    case CAS_ADDR:
    {
      onCasMessageReceived(frame);
      break;
    }
    //CIC
    case CIC_ADDR:
    {
      break;
    }
    case IDRIVE_CTRL_INIT_RESPONSE_ADDR:
    {
      iDriveInitSuccess = true;
      break;
    }
    case IDRIVE_CTRL_STATUS_ADDR:
    {
      onIdriveStatusReceived(frame);
      break;
    }
    //CAS: Schlüssel Buttons
    case CAS_ZV_ADDR:
    {
      onCasCentralLockingReceived(frame);
      break;
    }
    //RLS Lichtsensorik
    case RLS_ADDR:
    {
      onRainLightSensorReceived(frame);
      break;
    }
    //Licht-, Solar- und Innenraumtemperatursensoren
    case IHKA_ADDR:
    {
      //Byte #0 ist der Solarsensor mittig auf dem Armaturenbrett
      /*  
          Solarsensor auf Byte 0: Startet bei 0, in Praller Sonne wurde 73 zuletzt gemeldet. Das ist definitiv der Solar Sensor!
          Dummerweise wir der Sensor manchmal vom Displaygehäuse verdeckt, je nachdem wie die Sonne einfällt. Macht halt das ganze
          Kontrukt teilweise unbruachbar...
      */
      //Byte 4 ist der IR Sensor im Klimabedienteil. Offenbar arbeitet dieser auch mit den Zonensensoren mit.
      break;
    }
    //Steuerung für Helligkeit der Armaturenbeleuchtung (Abblendlicht aktiv)
    case LM_DIM_ADDR:
    {
      //254 = AUS
      //Bereich: 0-253
      //Ab und zu wird 254 einfach so geschickt, wenn 0 vorher aktiv war...warum auch immer
      /*       Serial.print("Beleuchtung (Roh, Ctrl):");
      int dimRawVal = frame.data[0];
      */
      break;
    }
    //Rückspiegel und dessen Lichtsensorik
    case SPIEGEL_ABBLEND_ADDR:
    {
      break;
    }

    //iDrive Controller

    //iDrive Controller: Drehung
    case IDRIVE_CTRL_ROT_ADDR:
    {
      onIdriveRotaryMovement(frame);
      break;
    } //END ROTARY

      //Knöpfe und Joystick
    case IDRIVE_CTRL_BTN_ADDR:
    {
      onIdriveButtonPressed(frame);
      break;
    }

    //PDC 1
    case PDC_ABSTANDSMELDUNG_1_ADDR:
    {
      /*       //Byte 0~3 = Hinten
      //Byte 4~7 = Vorne
      //Angaben in cm von 0 - 255
      //Heck:
      int backOuterLeft = frame.data[0];
      int backInnerLeft = frame.data[1];
      int backInnerRight = frame.data[2];
      int backOuterRight = frame.data[3];
      //Front:
      int frontOuterLeft = frame.data[4];
      int frontInnerLeft = frame.data[5];
      int frontInnerRight = frame.data[6];
      int frontOuterRight = frame.data[7];
 */
      break;
    }
    //Rückwärtsgang
    case LM_REVERSESTATUS_ADDR:
    {
      if (frame.data[0] == 0xFD)
      {
        //Rückwärtsgang NICHT aktiv
      }
      if (frame.data[0] == 0xFE)
      {
        //Rückwärtsgang AKTIV
      }
      break;
    }
    //Batteriespannung und Status
    case DMEDDE_POWERMGMT_BATTERY_ADDR:
    {
      //(((Byte[1]-240 )*256)+Byte[0])/68
      //float batteryVoltage = (((frame.data[1] - 240) * 256) + frame.data[0]) / 68;

      if (frame.data[3] == 0x00)
      {
        //Zeitstempel
        Serial.print(timeStamp + '\t');
        Serial.println("Engine RUNNING");
      }
      if (frame.data[3] == 0x09)
      {
        //Zeitstempel
        Serial.print(timeStamp + '\t');
        Serial.println("Engine OFF");
      }
      break;
    }
    //Uhrzeit
    case KOMBI_DATETIME_ADDR:
    {
      //Merken, wann das letzte mal diese Nachricht empfangen wurde.
      previousCanDateTime = millis();

      //0: Stunden
      hours = frame.data[0];
      //1: Minuten
      minutes = frame.data[1];
      //2: Sekunden
      seconds = frame.data[2];
      //3: Tage
      days = frame.data[3];
      //4: die ersten 4 bits stellen den Monat dar.
      month = frame.data[4] >> 4;
      //6 & 5: Jahr
      // #6 nach links shiften und 5 addieren
      year = (frame.data[6] << 8) + frame.data[5];

      buildtimeStamp();

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
    //Zeitstempel
    Serial.print(timeStamp + '\t');
    Serial.println("[checkCan] Keine Nachrichten seit 30 Sekunden. Der Bus wird nun als deaktiviert betrachtet.");
    enterPowerSaving();
  }
}

void enterPowerSaving()
{
  Serial.println("[Power] Entering power saving...");
  bleKeyboard.end();
  btStop();
  setCpuFrequencyMhz(10);
  if (odroidRunning == HIGH && !anyPendingActions())
  {
    stopOdroid();
  }
  hibernateActive = true;  
}

void exitPowerSaving()
{
  Serial.println("[Power] Exiting power saving mode...");
  if (!btStarted())
  {
    btStart();
    bleKeyboard.begin();
  }
  setCpuFrequencyMhz(240);
  hibernateActive = false;
}

void checkPins()
{
  //Staus Odroid Vcc pin
  odroidRunning = !digitalRead(PIN_ODROID_POWER_INPUT);

  //Status Debug-Pin
  debugMode = !digitalRead(PIN_DEBUG);

  //Prüfe alle Faktoren für Start, Stopp oder Pause des Odroid.
  checkIgnitionState();

  if(true)
  {
    Serial.print("[GPIO] Odroid: ");
    Serial.println(odroidRunning == 1 ? "Running" : "Stopped");
    Serial.print("[CAN] Ignition: ");
    Serial.println(ignitionOn == 0 ? "Running" : "Stopped");
  }
}

void checkIgnitionState()
{
  //Wenn der Status der Zündung sich verändert hat.
  if (ignitionOn != lastIgnitionState)
  {
    //Zündung ist an aber PC ist aus.
    if (ignitionOn == LOW && odroidRunning == LOW)
    {
      //PC starten.
      startOdroid();
    }
  }
  //Letzten Status merken.
  lastIgnitionState = ignitionOn;
}

void startOdroid()
{
  //Zeitstempel
  Serial.print(timeStamp + '\t');
  Serial.print("[startOdroid] Odroid Status:");
  Serial.println(odroidRunning == LOW ? "AUS" : "AN");
  //Mehrfachen Aufruf verhindern - auch wenn der PC bereits läuft
  if (odroidStartRequested || odroidRunning || pendingAction != NONE)
  {
    return;
  }

  //Starten
  odroidStartRequested = true;
  pendingAction = ODROID_START;
  digitalWrite(PIN_ODROID_POWER_BUTTON, HIGH);
  //Zeitstempel
  Serial.print(timeStamp + '\t');
  Serial.println("[startOdroid] Start angefordert.");
  previousOdroidActionTime = millis();
}

void pauseOdroid()
{
  //Wenn der PC aus ist, dann brauchen wir auch nicht Pause drücken.... Wir gehen auch einfach mal davon aus, dass er aus ist.
  //Wenn auch noch eine Stand-By Anforderung ausstehend ist, könnte ein erneutes Drücken den Start wieder auslösen.
  if (odroidPauseRequested || !odroidRunning || pendingAction != NONE)
  {
    return;
  }
  pendingAction = ODROID_STANDBY;
  digitalWrite(PIN_ODROID_DISPLAY_POWER_BUTTON, HIGH);
  digitalWrite(PIN_ODROID_POWER_BUTTON, HIGH);
  Serial.println("[pauseOdroid] Stand-By angefordert");
  //Kurze Verzögerung - kurzer Tastendruck für display und Odroid
  delay(ODROID_STANDBY_HOLD_DELAY);
  digitalWrite(PIN_ODROID_DISPLAY_POWER_BUTTON, LOW);
  digitalWrite(PIN_ODROID_POWER_BUTTON, LOW);
  odroidPauseRequested = true;
  previousOdroidPauseTime = millis();
}

void stopOdroid()
{
  //Zeitstempel
  Serial.print(timeStamp + '\t');
  Serial.print("[stopOdroid] Odroid Status:");
  Serial.println(odroidRunning == LOW ? "AUS" : "AN");
  //Mehrfachen Aufruf verhindern - auch wenn der PC bereits aus ist. Das würde diesen nämlich einschalten.
  if (odroidShutdownRequested || !odroidRunning || pendingAction != NONE)
  {
    return;
  }

  //Herunterfahren
  odroidShutdownRequested = true;
  pendingAction = ODROID_STOP;

  digitalWrite(PIN_ODROID_POWER_BUTTON, HIGH);
  Serial.println("[stopOdroid] Herunterfahren angefordert");

  previousOdroidActionTime = millis();
}

void buildtimeStamp()
{
  sprintf(timeStamp, "%02d:%02d:%02d %02d.%02d.%u", hours, minutes, seconds, days, month, year);
  sprintf(timeString, "%02d:%02d:%02d", hours, minutes, seconds);
  sprintf(dateString, "%02d.%02d.%u", days, month, year);
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

void timeKeeper()
{
  unsigned long currentMillis = millis();
  //Nur, wenn die letzte Aktualisierung über CAN mehr als eine Sekunde zurückliegt
  if (currentMillis - previousCanDateTime > 1000)
  {
    if (hours == 23 && minutes == 59 && seconds == 59)
    {
      hours = 0;
      minutes = 0;
      seconds = 0;
    }
    if (minutes == 59 && seconds == 59)
    {
      hours++;
      minutes = 0;
      seconds = 0;
    }
    if (seconds == 59)
    {
      minutes++;
      seconds = 0;
      buildtimeStamp();
      //Tick abgeschlossen
      return;
    }
    buildtimeStamp();
    //Sekunden erhöhen
    seconds++;
  }
}

void readConsole()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    if (command == "pc.stop")
    {
      stopOdroid();
    }
    if (command == "pc.start")
    {
      startOdroid();
    }
    if (command == "pc.pause")
    {
      pauseOdroid();
    }
    if (command == "car.testcan")
    {
    }
  }
}

bool sendMessage(int address, byte len, const uint8_t *buf)
{
  CANMessage frame;
  frame.id = address;
  frame.len = len;
  //These are here for reference only and are the default values of the ctr
  frame.ext = false;
  frame.rtr = false;
  frame.idx = 0;

  //Buffer in data kopieren.
  byte i = len;
  while (i--)
    *(frame.data + i) = *(buf + i);
  return can.tryToSend(frame);
}

bool setupCan()
{

  //Setup CAN Module
  SPI.begin(MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI);
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 100UL * 1000UL); // CAN bit rate 100 kb/s

  const uint16_t errorCode = can.begin(settings, []
                                       { can.isr(); });
  if (errorCode == 0)
  {
    Serial.print("Bit Rate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Propagation Segment: ");
    Serial.println(settings.mPropagationSegment);
    Serial.print("Phase segment 1: ");
    Serial.println(settings.mPhaseSegment1);
    Serial.print("Phase segment 2: ");
    Serial.println(settings.mPhaseSegment2);
    Serial.print("SJW: ");
    Serial.println(settings.mSJW);
    Serial.print("Triple Sampling: ");
    Serial.println(settings.mTripleSampling ? "yes" : "no");
    Serial.print("Actual bit rate: ");
    Serial.print(settings.actualBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact bit rate ? ");
    Serial.println(settings.exactBitRate() ? "yes" : "no");
    Serial.print("Sample point: ");
    Serial.print(settings.samplePointFromBitStart());
    Serial.println("%");
  }
  else
  {
    Serial.print("Configuration error 0x");
    Serial.println(errorCode, HEX);
  }

  previousCanMsgTimestamp = millis();

  return errorCode == 0;
}

void checkVoltage()
{
  float busvoltage = ina219.getBusVoltage_V();
  voltageReadings[voltageReadingCounter] = busvoltage;
  if (voltageReadingCounter == maxAverageReadings - 1)
  {
    voltageReadingCounter = 0;
  }
  else
  {
    voltageReadingCounter++;
  }

  float totalReadingsValue = 0.0F;
  for (int i = 0; i < maxAverageReadings; i++)
  {
    totalReadingsValue += voltageReadings[i];
  }

  float averageVoltageReading = totalReadingsValue / maxAverageReadings;

  if(true)
  {
    Serial.print("[Voltage] V: ");
    Serial.println(busvoltage);
    Serial.print("[Voltage] Avg: ");
    Serial.println(averageVoltageReading);
  }

  if (averageVoltageReading < 12.10F && odroidRunning == HIGH && !anyPendingActions())
  {
    stopOdroid();
  }
}

void onMflButtonPressed(CANMessage frame)
{
  //Kein Knopf gedrückt (alle 1000ms)
  if (frame.data[0] == 0xC0 && frame.data[1] == 0x0C)
  {
    //Taste wurde losgelassen und zuvor nicht lang genug gedrückt gehalten.
    if (mflButtonCounter < mflButtonHoldThreshold)
    {
      //NEXT oder PREV
      if (MflButtonNextPressed)
      {
        Serial.println("[MFL] NEXT");
        bleKeyboard.press(KEY_MEDIA_NEXT_TRACK);
        bleKeyboard.releaseAll();
      }
      if (MflButtonPrevPressed)
      {
        Serial.println("[MFL] PREV");
        bleKeyboard.press(KEY_MEDIA_PREVIOUS_TRACK);
        bleKeyboard.releaseAll();
      }
    }
    //Knöpfe zurücksetzen
    MflButtonNextPressed = false;
    MflButtonPrevPressed = false;
    //Zeitstempel merken
    lastMflRelease = currentMillis;
    //Counter zurücksetzen
    mflButtonCounter = 0;
  }

  //Wenn innerhalb einer Zeitspanne das selbe Tastensignal gesendet wird, wird der Knopf als gehalten betrachtet, aber nur wenn der Schwellwert erreicht wurde
  //Erneuter Tastendruck innerhalb von Zeit
  if (currentMillis - lastMflPress <= 200)
  {
    mflButtonCounter++;
    //Wenn der Knopf seit bestimmter Zeit nicht gelöst wurde und der Schwellwert erreicht wurde
    if (currentMillis - lastMflRelease >= mflButtonHoldTime && mflButtonCounter > mflButtonHoldThreshold)
    {
      //FASTFORWARD oder REWIND
      //Next
      if (frame.data[0] == 0xE0 && frame.data[1] == 0x0C)
      {
        Serial.println("[MFL] FASTFORWARD");
        bleKeyboard.press(KEY_MEDIA_TRACK_FASTFORWARD);
        bleKeyboard.releaseAll();
      }
      //Prev
      if (frame.data[0] == 0xD0 && frame.data[1] == 0x0C)
      {
        Serial.println("[MFL] REWIND");
        bleKeyboard.press(KEY_MEDIA_TRACK_REWIND);
        bleKeyboard.releaseAll();
      }
    }
  }
  //Next
  if (frame.data[0] == 0xE0 && frame.data[1] == 0x0C)
  {
    MflButtonNextPressed = true;
  }
  //Prev
  if (frame.data[0] == 0xD0 && frame.data[1] == 0x0C)
  {
    MflButtonPrevPressed = true;
  }

  lastMflPress = currentMillis;

  //Pickup Button
  if (frame.data[0] == 0xC1 && frame.data[1] == 0x0C)
  {
  }
  //Voice Button
  if (frame.data[0] == 0xC0 && frame.data[1] == 0x0D)
  {
  }
}

void onCasMessageReceived(CANMessage frame)
{
  //Wakeup Signal vom CAS --> Alle Steuergeräte aufwecken
  //Wird alle 100ms geschickt
  if (frame.data[0] == 0x45)
  {
    ignitionOn = LOW;
  }
  if (frame.data[0] == 0)
  {
    ignitionOn = HIGH;
  }

  //Wenn der Schlüssel im Fach ist, ist der Wert größer 0x0
  if (frame.data[0] > 0)
  {
    if (!iDriveInitSuccess)
    {
      bool sendResult = sendMessage(IDRIVE_CTRL_INIT_ADDR, 8, IDRIVE_CTRL_INIT);
      Serial.print("CAN Send OK:");
      Serial.println(sendResult);
    }
  }
}

void onIdriveStatusReceived(CANMessage frame)
{
  if (frame.data[4] == 6)
  {
    Serial.println("Controller ist nicht initialisiert.");
    //Controller meldet er sei nicht initialisiert: Nachricht zum Initialisieren senden.
    bool sendResult = sendMessage(IDRIVE_CTRL_INIT_ADDR, 8, IDRIVE_CTRL_INIT);
    Serial.print("CAN Send OK:");
    Serial.println(sendResult);
    iDriveInitSuccess = false;
  }
  else
  {
    iDriveInitSuccess = true;
  }
}

void onCasCentralLockingReceived(CANMessage frame)
{
  //Debounce: Befehle werden erst wieder verarbeitet, wenn der Timeout abgelaufen ist.
  if (currentMillis - previousCasMessageTimestamp > CAS_DEBOUNCE_TIMEOUT)
  {
    previousCasMessageTimestamp = currentMillis;
    //Öffnen:     00CF01FF
    if (frame.data[0] == 0x00 && frame.data[1] == 0x30 && frame.data[2] == 0x01 && frame.data[3] == 0x60)
    {
      //Prüfen, ob der PC noch im Begriff ist herunter zu fahren
      if (pendingAction == ODROID_STOP)
      {
        queuedAction = ODROID_START;
        Serial.println("[checkCan] PC wird nach dem Herunterfahren wieder gestartet.");
      }
      //Starten
      startOdroid();

      //Controller initialisieren.
      bool sendResult = sendMessage(IDRIVE_CTRL_INIT_ADDR, 8, IDRIVE_CTRL_INIT);
      Serial.print("CAN Send OK:");
      Serial.println(sendResult);
      previousIdriveInitTimestamp = currentMillis;
      //Zur Kontrolle die Instrumentenbeleuchtung einschalten.
      sendResult = sendMessage(DASHBOARD_LIGHTING_ADDR, 2, DASHBOARD_LIGHTING_ON);
      Serial.print("CAN Send OK:");
      Serial.println(sendResult);
      //Displayhelligkeit auf Maximum
      analogWrite(PIN_VU7A_BRIGHTNESS, 255);
    }
    //Schließen:  00DF40FF
    if (frame.data[0] == 0x00 && frame.data[1] == 0x30 && frame.data[2] == 0x04 && frame.data[3] == 0x60)
    {
      //Prüfen, ob der PC noch im Begriff ist hochzufahren
      if (pendingAction == ODROID_START)
      {
        queuedAction = ODROID_STOP;
        Serial.println("[checkCan] PC wird nach dem Starten wieder heruntergefahren.");
      }
      stopOdroid();
      //Displayhelligkeit auf vertretbares Minimum
      analogWrite(PIN_VU7A_BRIGHTNESS, 50);
    }
    //Kofferraum: Wird nur gesendet bei langem Druck auf die Taste
  }
}

void onRainLightSensorReceived(CANMessage frame)
{
  //Pralle Sonne:
  //[314]   50      0       FF
  //Tuch drüber gelegt:
  //[314]   11      8       FF

  //unsigned int lightVal = (frame.data[1] << 8) + frame.data[0];
  //Licht ist definitiv auf byte 0 aber keine Ahnung ob byte 1 noch was zu sagen hat. Dieses byte wechselt jedenfalls nach 8, wenn dann auch das Abblendlicht angeht.

  int lightValue = frame.data[0];

  //Lichtwert prüfen und bei Über- oder Unterschreitungen korrigieren.
  if (lightValue > MAX_LM_LIGHT_LEVEL)
  {
    lightValue = MAX_LM_LIGHT_LEVEL;
  }
  if (lightValue < MIN_LM_LIGHT_LEVEL)
  {
    lightValue = MIN_LM_LIGHT_LEVEL;
  }

  //Display auf volle Helligkeit einstellen. Das ist unser Basiswert
  int val = 255;

  val = map(lightValue, MIN_LM_LIGHT_LEVEL, MAX_LM_LIGHT_LEVEL, MIN_DISPLAY_BRIGHTNESS, MAX_DISPLAY_BRIGHTNESS);
  Serial.print("Licht (Roh, CTRL):\t");
  Serial.print(lightValue);
  Serial.print('\t');
  Serial.print(frame.data[1]);
  Serial.print('\t');
  Serial.println(val);

  //Wenn der aktuelle Wert größer als der zuletzt gespeicherte ist, zählen wir vom letzten Wert hoch.
  if (val > lastBrightness)
  {
    for (int i = lastBrightness; i <= val; i++)
    {
      analogWrite(PIN_VU7A_BRIGHTNESS, i);
      delay(10);
    }
  }

  //Wenn der aktuelle Wert kleiner als der zuletzt gespeicherte ist, dann schrittweise die Helligkeit vom letzten bekannten Wert absenken
  if (val < lastBrightness)
  {
    for (int i = lastBrightness; i >= val; i--)
    {
      analogWrite(PIN_VU7A_BRIGHTNESS, i);
      delay(10);
    }
  }

  //Wenn der Wert unverändert ist zur Sicherheit nochmals letzten Wert schreiben.
  if (val == lastBrightness)
  {
    analogWrite(PIN_VU7A_BRIGHTNESS, val);
    return;
  }

  //letzten Wert zum Vergleich speichern
  lastBrightness = val;
}

void onIdriveRotaryMovement(CANMessage frame)
{
  //Byte 2 beinhaltet den counter
      //Byte 3 Counter Geschwindigkeit der Drehrichtung:
      //        Startet bei 0 bei Drehung im Uhrzeigersinn, wird von 0xFE heruntergezählt bei entgegengesetzter Richtung.
      //Byte 4 0x80 für Drehung im Uhrzeigersinn
      //       0x7F für Drehung gegen den Uhrzeigersinn
      //        Alle anderen Werte: Keine Drehung

      //Code von IAmOrion
      //  https://github.com/IAmOrion/BMW-iDrive-BLE-HID

      //Das System arbeitet nach LittleEndian. Byte 4 und 3 repräsentieren die Drehung und somit auch Drehrichtung.
      /*
      Beispiel:
      E1      FD      AA      FE      7F      1E
      E1      FD      AB      FD      7F      1E
      E1      FD      AC      FE      7F      1E
      E1      FD      AD      FF      7F      1E
      E1      FD      AE      1       80      1E
      E1      FD      AF      2       80      1E
      E1      FD      B0      3       80      1E
      */
      //Es wird also von 80FF nach 7F00 heruntergezählt und umgekehrt.

      byte rotarystepa = frame.data[3];
      byte rotarystepb = frame.data[4];
      //unsigned int newpos = (((unsigned int)rotarystepa) + ((unsigned int)rotarystepb) * 0x100);
      //Bitshift Endianness: 0xFF, 0x7F -> 7FFF
      unsigned int newpos = (rotarystepb << 8) + rotarystepa;

      //Initialstellung des Encoders feststellen
      if (!(RotaryInitPositionSet))
      {
        switch (rotarystepb)
        {
        case 0x7F:
          rotaryposition = (newpos + 1);
          break;
        case 0x80:
          rotaryposition = (newpos - 1);
          break;
        default:
          rotaryposition = newpos;
          break;
        }
        RotaryInitPositionSet = true;
      }

      //Da auch die Drehgeschwindigkeit durch byte 3 mit einbezogen wird, sollte diese auch ausgeführt werden.
      //Hier wird einfach das Delta zwischen alter und neuer Position ausgeführt.
      while (rotaryposition < newpos)
      {
        if (iDriveInitSuccess)
        {
          iDriveRotDir = ROTATION_LEFT;
          //Während der Menüknopf gedrückt ist (bzw. der Joystick gedrückt ist) kann man mit Drehung des Joysticks blättern
          if (iDriveBtnPress == BUTTON_LONG_PRESS && lastKnownIdriveButtonType == IDRIVE_BUTTON_CENTER_MENU)
          {
            //Taste drücken und ALT gedrückt halten, danach Taste wieder loslassen aber ALT gedrückt halten
            bleKeyboard.press(KEY_LEFT_ARROW);
            bleKeyboard.press(KEY_LEFT_ALT);
            bleKeyboard.release(KEY_LEFT_ALT);
          }
          else
          {
            bleKeyboard.press(KEY_DOWN_ARROW);
            bleKeyboard.release(KEY_DOWN_ARROW);
          }
        }
        rotaryposition++;
      }
      while (rotaryposition > newpos)
      {
        if (iDriveInitSuccess)
        {
          iDriveRotDir = ROTATION_RIGHT;
          if (iDriveBtnPress == BUTTON_LONG_PRESS && lastKnownIdriveButtonType == IDRIVE_BUTTON_CENTER_MENU)
          {
            //Taste drücken und ALT gedrückt halten, danach Taste wieder loslassen aber ALT gedrückt halten
            bleKeyboard.press(KEY_RIGHT_ARROW);
            bleKeyboard.press(KEY_LEFT_ALT);
            bleKeyboard.release(KEY_LEFT_ALT);
          }
          else
          {
            bleKeyboard.press(KEY_UP_ARROW);
            bleKeyboard.release(KEY_UP_ARROW);
          }
        }
        rotaryposition--;
      }
      //Zeitstempel
      Serial.print(timeStamp + '\t');
      Serial.print("[checkCan] iDrive Rotation: ");
      switch (iDriveRotDir)
      {
      case ROTATION_LEFT:
      {
        Serial.println("Rechtsdrehung");
        break;
      }
      case ROTATION_RIGHT:
      {
        Serial.println("Linksdrehung");
        break;
      }

      default:
      {
        Serial.println("Keine");
        break;
      }
      } //END ROTARY DIRECTION
}

void onIdriveButtonPressed(CANMessage frame)
{
  //Dieser Wert erhöht sich, wenn eine Taste gedrückt wurde.
      int buttonCounter = frame.data[2];

      //Status der Taste (kurz, lang, losgelassen) oder Joystick-Richtung
      int buttonPressType = frame.data[3];

      //Eingabetyp: Button oder Joystick
      int inputType = frame.data[4];

      //Knopf
      int buttonType = frame.data[5];

      //Entprellung der Knöpfe: Bei jedem Tastendruck wird eine Laufnummer auf byte 2 gesendet. Solange diese sich nicht verändert, wird der Knopf gehalten.
      //Zur Sicherheit wird dabei gleichzeitig die ID des Knopfes selbst abgeglichen.
      if ((buttonCounter != previousIdriveButtonPressCounter || lastKnownIdriveButtonPressType != buttonPressType) && previousIdriveButtonTimestamp - currentMillis >= IDRIVE_BUTTON_DEBOUNCE)
      {
        //Fallunterscheidung nach Art des Knopfdrucks:
        // Kurzer Druck = 1 (Wird dauerhaft gesendet)
        // Gehalten = 2 (Wird nach ca 2 Sekunden halten gesendet)
        // Losgelassen = 0 (wird immer nach dem Loslassen gesendet)
        switch (buttonPressType)
        {
        //Kurzer Knopfdruck registriert
        case 0x01:
        {
          iDriveBtnPress = BUTTON_SHORT_PRESS;
          break;
        }
        //Lang
        case 0x02:
        {
          iDriveBtnPress = BUTTON_LONG_PRESS;
          break;
        }
        } //END BUTONPRESSTYPE
      }
      else
      {
        previousIdriveButtonTimestamp = currentMillis;
        return;
      }

      //Egal wie der vorherige Status war wird beim Senden von "0" die Taste als losgelassen betrachtet.
      if (buttonPressType == 0x00)
      {
        iDriveBtnPress = BUTTON_RELEASE;
        Serial.println("[iDrive] RELEASE");
      }

      //Zeitstempel des letzten Knopfdrucks merken.
      previousIdriveButtonTimestamp = currentMillis;
      //Zuletzt empfangenen Zähler merken.
      previousIdriveButtonPressCounter = buttonCounter;
      //Zuletzt empfangene Bedienungsart merken.
      lastKnownIdriveButtonPressType = buttonPressType;
      //Zuletzt gedrücktne Knopf merken
      lastKnownIdriveButtonType = buttonType;

      //Aussortieren, ob der Knopf in eine Richtung gedrückt wurde oder ob ein Funktionsknopf gedrückt wurde.
      if (inputType != IDRIVE_JOYSTICK)
      {        
        //Knöpfe entsprechend nach Typ behandeln
        switch (buttonType)
        {
        //Joystick oder Menüknopf
        case IDRIVE_BUTTON_CENTER_MENU:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            bleKeyboard.press(KEY_RETURN);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            //BPMod->keyboardPress(BP_KEY_F11, BP_MOD_NOMOD);
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            bleKeyboard.release(KEY_RETURN);
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
          //BACK Button
        case IDRIVE_BUTTON_BACK:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            //Zurück
            bleKeyboard.press(KEY_ESC);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
            //BPMod->keyboardPress(BP_KEY_F10, BP_MOD_NOMOD);
            break;
          //Losgelassen
          case BUTTON_RELEASE:
          {
            bleKeyboard.release(KEY_ESC);
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
          //OPTION Button
        case IDRIVE_BUTTON_OPTION:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            //Menü aufrufen
            bleKeyboard.press(KEY_F9);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            //BPMod->keyboardPress(BP_KEY_F11, BP_MOD_NOMOD);
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            bleKeyboard.release(KEY_F9);
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
          //RADIO Button
        case IDRIVE_BUTTON_RADIO:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            bleKeyboard.press(KEY_F5);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            bleKeyboard.release(KEY_F5);
            break;
          }

          } //END BUTTON PRESS DURATION
          break;
        }
          //CD Button
        case IDRIVE_BUTTON_CD:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            bleKeyboard.press(KEY_F6);
            bleKeyboard.release(KEY_F6);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
          //NAV Button
        case IDRIVE_BUTTON_NAV:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            bleKeyboard.press(KEY_F7);
            bleKeyboard.release(KEY_F7);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
          //TEL Button
        case IDRIVE_BUTTON_TEL:
        {
          switch (iDriveBtnPress)
          {
          //Kurz gedrückt
          case BUTTON_SHORT_PRESS:
          {
            bleKeyboard.press(KEY_F8);
            bleKeyboard.release(KEY_F8);
            break;
          }
          //Lang gedrückt
          case BUTTON_LONG_PRESS:
          {
            break;
          }
          //Losgelassen
          case BUTTON_RELEASE:
          {
            break;
          }
          } //END BUTTON PRESS DURATION
          break;
        }
        default:
        {
          break;
        } //END BUTTON PRESS
        }
        previousIdriveButton = buttonType;
      }
      else
      {
        switch (buttonPressType)
        {
          //Hoch (kurz)
        case IDRIVE_JOYSTICK_UP:
          bleKeyboard.press(KEY_UP_ARROW);
          bleKeyboard.release(KEY_UP_ARROW);
          break;
          //Hoch (lang)
        case IDRIVE_JOYSTICK_UP_HOLD:
          break;
        //Rechts (kurz)
        case IDRIVE_JOYSTICK_RIGHT:
          bleKeyboard.press(KEY_RIGHT_ARROW);
          bleKeyboard.release(KEY_RIGHT_ARROW);
          break;
        //Rechts (lang)
        case IDRIVE_JOYSTICK_RIGHT_HOLD:
          break;
        //Runter (kurz)
        case IDRIVE_JOYSTICK_DOWN:
          bleKeyboard.press(KEY_DOWN_ARROW);
          bleKeyboard.release(KEY_DOWN_ARROW);
          break;
        //Runter (lang)
        case IDRIVE_JOYSTICK_DOWN_HOLD:
          break;
        //Links (kurz)
        case IDRIVE_JOYSTICK_LEFT:
          bleKeyboard.press(KEY_LEFT_ARROW);
          bleKeyboard.release(KEY_LEFT_ARROW);
          break;
        //Links (lang)
        case IDRIVE_JOYSTICK_LEFT_HOLD:
          break;
        default:
          break;
        } //END DIRECTION
      }
}

bool anyPendingActions()
{
  return odroidStartRequested || odroidShutdownRequested || odroidPauseRequested;
}