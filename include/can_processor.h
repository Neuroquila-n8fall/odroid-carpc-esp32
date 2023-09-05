#ifndef _CAN_PROCESSOR_H
#define _CAN_PROCESSOR_H

#include <Arduino.h>
#include <k-can.h>

struct CANMessage {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
} frame;

//CAN Nachrichten auf der Konsole ausgeben
extern void printCanMsg(int canId, unsigned char *buffer, int len);
//CAN Output als CSV
extern void printCanMsgCsv(int canId, unsigned char *buffer, int len);
extern bool sendMessage(int address, byte len, const uint8_t *buf);
extern void processCanMessages();
//CAN Initialisieren
extern bool setupCan();


//Process status messages from iDrive Controller
extern void onIdriveStatusReceived(CANMessage frame);
//Process messages sent by the rotary encoder of the iDrive controller
extern void onIdriveRotaryMovement(CANMessage frame);
//Process messages sent by the button encoder of the iDrive controller
extern void onIdriveButtonPressed(CANMessage frame);

#endif