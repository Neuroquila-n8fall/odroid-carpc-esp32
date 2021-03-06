#ifndef _CAN_PROCESSOR_H
#define _CAN_PROCESSOR_H

#include <Arduino.h>
#include <settings.h>
#include <ble_keyboard.h>
#include <pins.h>
#include <k-can-addresses.h>
#include <k-can-messages.h>
#include <k-can.h>
#include <idrive-controls.h>

struct CANMessage {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
} frame;

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Driver object
//——————————————————————————————————————————————————————————————————————————————


//CAN Nachrichten auf der Konsole ausgeben
extern void printCanMsg(int canId, unsigned char *buffer, int len);
//CAN Output als CSV
extern void printCanMsgCsv(int canId, unsigned char *buffer, int len);
extern bool sendMessage(int address, byte len, const uint8_t *buf);
extern void processCanMessages();
//CAN Initialisieren
extern bool setupCan();

//Process MFL Button Press
extern void onMflButtonPressed(CANMessage frame);
//Process messages from CAS
extern void onCasMessageReceived(CANMessage frame);
//Process status messages from iDrive Controller
extern void onIdriveStatusReceived(CANMessage frame);
//Process status messages from remote fob and central locking
extern void onCasCentralLockingReceived(CANMessage frame);
//Process messages sent by the rain and light sensor
extern void onRainLightSensorReceived(CANMessage frame);
//Process messages sent by the rotary encoder of the iDrive controller
extern void onIdriveRotaryMovement(CANMessage frame);
//Process messages sent by the button encoder of the iDrive controller
extern void onIdriveButtonPressed(CANMessage frame);

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Quartz: adapt to your design
//——————————————————————————————————————————————————————————————————————————————
static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL; // 16 MHz

#endif