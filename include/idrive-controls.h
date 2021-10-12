#ifndef _IDRIVE_CONTROLS_H
#define _IDRIVE_CONTROLS_H

#include <Arduino.h>

enum iDriveControls
{
    //Menü- oder Drehknopf gedrückt
    IDRIVE_BUTTON_CENTER_MENU = 0x01,
    //Back
    IDRIVE_BUTTON_BACK = 0x02,
    //Option
    IDRIVE_BUTTON_OPTION = 0x04,
    //Radio
    IDRIVE_BUTTON_RADIO = 0x08,
    //CD
    IDRIVE_BUTTON_CD = 0x10,
    //NAV
    IDRIVE_BUTTON_NAV = 0x20,
    //TEL
    IDRIVE_BUTTON_TEL = 0x40,
    //Drehknopf wurde in eine Richtung bewegt
    IDRIVE_JOYSTICK = 0xDD,

    //Joystick hoch
    IDRIVE_JOYSTICK_UP = 0x11,
    //Joystick hoch gehalten
    IDRIVE_JOYSTICK_UP_HOLD = 0x12,
    //Joystick rechts
    IDRIVE_JOYSTICK_RIGHT = 0x21,
    //Joystick rechts gehalten
    IDRIVE_JOYSTICK_RIGHT_HOLD = 0x22,
    //Joystick runter
    IDRIVE_JOYSTICK_DOWN = 0x41,
    //Joystick runter gehalten
    IDRIVE_JOYSTICK_DOWN_HOLD = 0x42,
    //Joystick links
    IDRIVE_JOYSTICK_LEFT = 0x81,
    //Joystick links gehalten
    IDRIVE_JOYSTICK_LEFT_HOLD = 0x82,

    //Adresse für Touch input
    IDRIVE_CTRL_TOUCH_ADDR = 0x0BF,
    //Touch-Modes
    //Keine Finger
    IDRIVE_CTRL_TOUCH_RELEASE = 0x11,
    //Ein Finger
    IDRIVE_CTRL_TOUCH_FINGER = 0x10,
    //Zwei Finger
    IDRIVE_CTRL_TOUCH_MULTI = 0x20,
};

#endif
