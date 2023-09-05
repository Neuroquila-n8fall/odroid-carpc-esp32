#ifndef _BLECONTROLLER_H
#define _BLECONTROLLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduinojson.h>

static const char* CHARACTERISTIC_UUID = "657759e4-8677-411d-978e-08d204ed3c76";
static const char* SERVICE_UUID = "5c301cef-8ff1-4a9a-91be-f2aba0dd35d2";

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

class SettingsCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic);
    void onRead(BLECharacteristic *pCharacteristic);
};


private:
    BLEServer* pServer;
    BLECharacteristic* pCharacteristic;
    static bool deviceConnected;
    static bool oldDeviceConnected;
    static bool devicePaired;
    static char advertisedName[16];
    static int securityPin;
    static void onConnect(BLEServer* pServer);
    static void onDisconnect(BLEServer* pServer);
};

#endif