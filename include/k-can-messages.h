#ifndef _K_CAN_MESSAGES_H
#define _K_CAN_MESSAGES_H

#include <Arduino.h>

//Nachricht für Armaturenbeleuchtung einschalten
extern uint8_t DASHBOARD_LIGHTING_ON[2];
//Nachricht für Armaturenbeleuchtung auszuschalten
extern uint8_t DASHBOARD_LIGHTING_OFF[2];

//Nachricht für Keepalive
extern uint8_t IDRIVE_CTRL_KEEPALIVE_KCAN2[8];

//Nachricht für Keepalive (KCAN)
extern uint8_t IDRIVE_CTRL_KEEPALIVE[8];

//Nachricht für Rotary Init
extern uint8_t IDRIVE_CTRL_INIT[8];

#endif