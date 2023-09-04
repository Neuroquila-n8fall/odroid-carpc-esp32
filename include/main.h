#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>

#include <can_processor.h>

/*------------- Functions -------------*/

// Interaktion mit serieller Konsole
extern void readConsole();
// CAN Nachricht senden.
extern bool sendMessage(int address, byte len, const uint8_t *buf);
// Power Saver
extern void enterPowerSaving();
// Exit Power Saving
extern void exitPowerSaving();
// Returns if any pending actions are queued
extern bool anyPendingActions();

extern void onIgnitionStatusReceived(CANMessage frame);
extern void onIndicatorStatusReceived(CANMessage frame);
extern void onIndicatorStalkReceived(CANMessage frame);
extern void onDriverDoorStatusReceived(CANMessage frame);
extern void onPassengerDoorStatusReceived(CANMessage frame);
extern void showIndicator();
extern void onCasCentralLockingReceived(CANMessage frame);

#endif