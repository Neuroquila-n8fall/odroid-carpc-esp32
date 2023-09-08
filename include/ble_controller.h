#ifndef _BLECONTROLLER_H
#define _BLECONTROLLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduinojson.h>
#include <persistence.h>

static const char* READ_CHARACTERISTIC_UUID = "657759e4-8677-411d-978e-08d204ed3c76";
static const char* WRITE_CHARACTERISTIC_UUID = "657759e4-8677-411d-978e-08d204ed3c77";
static const char* STATUS_CHARACTERISTIC_UUID = "657759e4-8677-411d-978e-08d204ed3c78";
static const char* SERVICE_UUID = "657759e4-8677-411d-978e-08d204ed3c70";

class BLEController {
public:
    BLEController();
    void init(char *deviceName, int pin);
    void handleClient();
    static bool isDeviceConnected();
    static bool wasDeviceConnected();
    static void setOldDeviceConnected(bool status);

class MySecurityCallbacks : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest();
    void onPassKeyNotify(uint32_t pass_key);
    bool onConfirmPIN(uint32_t pass_key);
    bool onSecurityRequest();
    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl);
};

class MyBleCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
};

class SettingsCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic);
    void onRead(BLECharacteristic *pCharacteristic);
};

class StatusCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic);
};


private:
    BLEServer* pServer;
    BLECharacteristic* pStatusCharacteristic;
    BLECharacteristic* pReadCharacteristic;
    BLECharacteristic* pWriteCharacteristic;
    static bool deviceConnected;
    static bool oldDeviceConnected;
    static bool devicePaired;
    static char advertisedName[16];
    static int securityPin;
    static DynamicJsonDocument responseDoc;
};

#endif