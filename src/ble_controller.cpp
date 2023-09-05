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
    pServer->setCallbacks(new BLEServerCallbacks());

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

    pStatusCharacteristic->setCallbacks(new StatusCharacteristicCallbacks());
    pReadCharacteristic->setCallbacks(new SettingsCharacteristicCallbacks());
    pWriteCharacteristic->setCallbacks(new SettingsCharacteristicCallbacks());
    pService->start();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
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

void BLEController::onConnect(BLEServer *pServer)
{
    if (devicePaired)
    {
        pServer->disconnect(pServer->getConnId()); // Disconnect if a device is already paired
        return;
    }
    deviceConnected = true;
}

void BLEController::onDisconnect(BLEServer *pServer)
{
    deviceConnected = false;
    pServer->getAdvertising()->start();
}

uint32_t BLEController::MySecurityCallbacks::onPassKeyRequest()
{
    return securityPin; // You can set your own passkey here
}

void BLEController::MySecurityCallbacks::onPassKeyNotify(uint32_t pass_key)
{
    // Handle passkey notification if needed
}

bool BLEController::MySecurityCallbacks::onConfirmPIN(uint32_t pass_key)
{
    return false; // For now, we won't use PIN confirmation
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
    }
}

void BLEController::SettingsCharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
    // Create a JSON object to hold the preferences
    DynamicJsonDocument doc(1024);

    // Populate the JSON object with preferences
    doc["deviceName"] = deviceName;
    doc["securityPin"] = securityPin;
    doc["activeProfile"] = static_cast<int>(activeProfile);
    doc["brightness"] = brightness;
    doc["color_red"] = color_red;
    doc["color_green"] = color_green;
    doc["color_blue"] = color_blue;
    doc["backLeds"] = backLeds;
    doc["centerLeds"] = centerLeds;
    doc["frontLeds"] = frontLeds;
    doc["updatesPerSecond"] = updatesPerSecond;

    // Convert the JSON object to a string
    String jsonString;
    serializeJson(doc, jsonString);

    // Set the characteristic value to the JSON string
    pCharacteristic->setValue(jsonString.c_str());
}

void BLEController::SettingsCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    // Get the written value (JSON string)
    std::string value = pCharacteristic->getValue();

    // Parse the JSON string
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, value.c_str());

    // Update the preferences based on the received JSON
    if (doc.containsKey("deviceName"))
    {
        strncpy(deviceName, doc["deviceName"], sizeof(deviceName));
    }

    if (doc.containsKey("securityPin"))
    {
        securityPin = doc["securityPin"];
    }

    if (doc.containsKey("activeProfile"))
    {
        activeProfile = static_cast<Profiles>(doc["activeProfile"].as<int>());
    }

    if (doc.containsKey("brightness"))
    {
        brightness = doc["brightness"];
    }

    if (doc.containsKey("color_red"))
    {
        color_red = doc["color_red"];
    }

    if (doc.containsKey("color_green"))
    {
        color_green = doc["color_green"];
    }

    if (doc.containsKey("color_blue"))
    {
        color_blue = doc["color_blue"];
    }

    if (doc.containsKey("backLeds"))
    {
        backLeds = doc["backLeds"];
    }

    if (doc.containsKey("centerLeds"))
    {
        centerLeds = doc["centerLeds"];
    }

    if (doc.containsKey("frontLeds"))
    {
        frontLeds = doc["frontLeds"];
    }

    if (doc.containsKey("updatesPerSecond"))
    {
        updatesPerSecond = doc["updatesPerSecond"];
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
