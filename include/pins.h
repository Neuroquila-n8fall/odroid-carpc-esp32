#ifndef _PINS_H
#define _PINS_H

#include <Arduino.h>

/*------------- PIN DEFINITIONS -------------*/
//Opto 2 - Zündung Aktiv
const int PIN_IGNITION_INPUT = GPIO_NUM_12;
//Eingang Odroid Power Button. Wenn dieser HIGH ist, ist der Odroid aktiv.
const int PIN_ODROID_POWER_INPUT = GPIO_NUM_16;
//LED PSU Enable Pin. HIGH = OFF, LOW = ON
const int PIN_LED_PSU_ENABLE = GPIO_NUM_27;
//Power Button vom Display
const int PIN_ODROID_DISPLAY_POWER_BUTTON = GPIO_NUM_15;
//Pin für Display Helligkeit
const int PIN_VU7A_BRIGHTNESS = GPIO_NUM_14;
//Pin für Debug Switch
const int PIN_DEBUG = GPIO_NUM_4;
//LED_BUILTIN existiert nicht auf dem ESP32. Daher setzen wir hier unseren eigenen Pin dafür
const int LED_BUILTIN = GPIO_NUM_33;

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 SPI Pin Assignment
//——————————————————————————————————————————————————————————————————————————————

static const byte MCP2515_SCK = GPIO_NUM_18;  // SCK input of MCP2515
static const byte MCP2515_MOSI = GPIO_NUM_23; // SDI input of MCP2515
static const byte MCP2515_MISO = GPIO_NUM_19; // SDO output of MCP2515
static const byte MCP2515_CS = GPIO_NUM_5;    // CS input of MCP2515 (adapt to your design)
static const byte MCP2515_INT = GPIO_NUM_17;  // INT output of MCP2515 (adapt to your design)

/*    
        EN  - X  	    23   - CAN-SI
I-Only  36  -   	    22   - SCL
I-Only  39  -   	    1    - UART0 TX
I-Only  34  -   	    3    - UART0 RX
I-Only  35  -   	    21   - SDA
        32  - PWM     19   - CAN-SO
        33  - O  	    18   - CAN-SCK
DAC1    25  -   	    5    - CAN-CS
DAC2    26  -   	    17   - CAN-INT (UART1 TX)
        27  -   	    16   - UART1 RX
        14  - O  	    4    - InPu
        12  - I  	    2    -
        13  - I  	    15   - O
        9	  - X       0 
        10  - X  	    8    - X
        11  - X  	    7    - X
        GND - X  	    6    - X
        VIN - X  	    3V3  - X
*/

#endif