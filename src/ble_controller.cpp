#include "ble_controller.h"
#include "led-control.h"

bool BLEController::deviceConnected = false;
bool BLEController::oldDeviceConnected = false;
bool BLEController::devicePaired = false;

BLEController::BLEController()
{
    // Constructor code if needed
}

void BLEController::init()
{
    BLEDevice::init("ESP32_LED_Controller");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BLEServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID); // Assuming you've defined SERVICE_UUID
    // Create a BLE Characteristic for exchanging JSON payloads
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new BLECharacteristicCallbacks());
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
}

uint32_t BLEController::MySecurityCallbacks::onPassKeyRequest()
{
    return 123456; // You can set your own passkey here
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
    DynamicJsonDocument doc(512); // Create a JSON document. Adjust the size if needed.

    doc["profile"] = static_cast<int>(activeProfile);

    doc["brightness"] = brightness;
    JsonObject color = doc.createNestedObject("color");

    color["r"] = color_red;
    color["g"] = color_green;
    color["b"] = color_blue;

    std::string response;
    serializeJson(doc, response);
    pCharacteristic->setValue(response);
}

bool BLEController::isDeviceConnected() {
    return deviceConnected;
}

bool BLEController::wasDeviceConnected() {
    return oldDeviceConnected;
}

void BLEController::setOldDeviceConnected(bool status) {
    oldDeviceConnected = status;
}

