#include "ble_controller.h"
#include "led-control.h"

bool BLEController::deviceConnected = false;
bool BLEController::oldDeviceConnected = false;
bool BLEController::devicePaired = false;
char BLEController::advertisedName[16];
int BLEController::securityPin = 123456;
DynamicJsonDocument BLEController::responseDoc(128);

BLEController::BLEController()
{
}

void BLEController::init(char *deviceName, int pin)
{
    strcpy(advertisedName, deviceName);
    securityPin = pin;

    BLEDevice::init(advertisedName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyBleCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    // Create a BLE Characteristic for sending commands from the app to the ESP32
    BLECharacteristic *pCommandCharacteristic = pService->createCharacteristic(
        WRITE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE);

    // Create a BLE Characteristic for the app to read data from the ESP32
    BLECharacteristic *pDataCharacteristic = pService->createCharacteristic(
        READ_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    // Create a BLE Characteristic for the ESP32 to provide feedback about the last command
    BLECharacteristic *pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    // Require pairing for characteristics
    pCommandCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
    pDataCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
    pStatusCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);

    // Set the characteristic callbacks
    pStatusCharacteristic->setCallbacks(new StatusCharacteristicCallbacks());
    pDataCharacteristic->setCallbacks(new SettingsCharacteristicCallbacks());
    pCommandCharacteristic->setCallbacks(new SettingsCharacteristicCallbacks());

    // Set up security
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);    
    pSecurity->setCapability(ESP_IO_CAP_KBDISP);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setStaticPIN(securityPin);
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

    // Start the service
    pService->start();

    // Set up advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void BLEController::handleClient()
{
    if (deviceConnected) {
        // Save the current connection state
        oldDeviceConnected = deviceConnected;
    }

    // If no device is currently connected and the previous state was connected, start advertising
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // Optional delay to give BLE stack some time
        BLEDevice::startAdvertising();
        Serial.println("Restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }
}

void BLEController::MyBleCallbacks::onConnect(BLEServer *pServer)
{
    deviceConnected = true;
}

void BLEController::MyBleCallbacks::onDisconnect(BLEServer *pServer)
{
    deviceConnected = false;
}

uint32_t BLEController::MySecurityCallbacks::onPassKeyRequest()
{
    return securityPin;
}

void BLEController::MySecurityCallbacks::onPassKeyNotify(uint32_t pass_key)
{
    // Handle passkey notification if needed
}

bool BLEController::MySecurityCallbacks::onConfirmPIN(uint32_t pass_key)
{
    return securityPin == pass_key; 
}

bool BLEController::MySecurityCallbacks::onSecurityRequest()
{
    return true;
}

void BLEController::MySecurityCallbacks::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl)
{
    if (cmpl.success)
    {
        devicePaired = true;            
        BLEDevice::stopAdvertising();
    }
}

void BLEController::SettingsCharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
    // Create a JSON object to hold the preferences
    DynamicJsonDocument doc(1024);

    // Populate the JSON object with preferences
    doc["DeviceName"] = deviceName;
    doc["SecurityPin"] = securityPin;
    doc["ActiveProfile"] = static_cast<int>(activeProfile);
    doc["Brightness"] = brightness;
    doc["ColorRed"] = color_red;
    doc["ColorGreen"] = color_green;
    doc["ColorBlue"] = color_blue;
    doc["LedCount"] = ledCount[0];    
    doc["BackLeds"] = backLeds;
    doc["CenterLeds"] = centerLeds;
    doc["FrontLeds"] = frontLeds;
    doc["UpdatesPerSecond"] = updatesPerSecond;

    // Convert the JSON object to a string
    String jsonString;
    serializeJson(doc, jsonString);

    // Set the characteristic value to the JSON string
    pCharacteristic->setValue(jsonString.c_str());
}

void BLEController::SettingsCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    // Get the written value (JSON string)
    String value = pCharacteristic->getValue().c_str();

    // Parse the JSON string
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, value.c_str());

    // Update the preferences based on the received JSON
    if (doc.containsKey("DeviceName"))
    {
        strncpy(deviceName, doc["DeviceName"], sizeof(deviceName));
    }

    if (doc.containsKey("SecurityPin"))
    {
        securityPin = doc["SecurityPin"];
    }

    if (doc.containsKey("ActiveProfile"))
    {
        activeProfile = static_cast<Profiles>(doc["ActiveProfile"].as<int>());
    }

    if (doc.containsKey("Brightness"))
    {
        brightness = doc["Brightness"];
    }

    if (doc.containsKey("ColorRed"))
    {
        color_red = doc["ColorRed"];
    }

    if (doc.containsKey("ColorGreen"))
    {
        color_green = doc["ColorGreen"];
    }

    if (doc.containsKey("ColorBlue"))
    {
        color_blue = doc["ColorBlue"];
    }

    if(doc.containsKey("LedCount"))
    {
        ledCount[0] = doc["LedCount"][0];
        ledCount[1] = doc["LedCount"][1];
    }

    if (doc.containsKey("BackLeds"))
    {
        backLeds = doc["BackLeds"];
    }

    if (doc.containsKey("CenterLeds"))
    {
        centerLeds = doc["CenterLeds"];
    }

    if (doc.containsKey("FrontLeds"))
    {
        frontLeds = doc["FrontLeds"];
    }

    if (doc.containsKey("UpdatesPerSecond"))
    {
        updatesPerSecond = doc["UpdatesPerSecond"];
    }

    // Update the userColor based on the new RGB values
    userColor = CRGB(color_red, color_green, color_blue);

    // Write the preferences to the storage
    writeLedProfile();
    writeLedBrightness();
    writeLedColor();
    writeLedCount();
    writeUpdatesPerSecond();
    writeSecurityPin();
    writeDeviceName();

    DynamicJsonDocument responseDoc(128);

    // Restart the ESP if the reboot flag is set
    if (doc.containsKey("reboot"))
    {
        ESP.restart();
    }

    // If we are not rebooting, then we will send an OK response so the app knows the controller received the message
    responseDoc["success"] = true;
}

void BLEController::StatusCharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
    // Convert the JSON object to a string
    String jsonString;
    serializeJson(responseDoc, jsonString);

    // Set the characteristic value to the JSON string
    pCharacteristic->setValue(jsonString.c_str());
}

bool BLEController::isDeviceConnected()
{
    return deviceConnected;
}

bool BLEController::wasDeviceConnected()
{
    return oldDeviceConnected;
}

void BLEController::setOldDeviceConnected(bool status)
{
    oldDeviceConnected = status;
}
