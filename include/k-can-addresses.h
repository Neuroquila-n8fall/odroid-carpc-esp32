#ifndef _K_CAN_ADDRESSES_H
#define _K_CAN_ADDRESSES_H

#include <Arduino.h>

enum kCanAddresses {
//Adresse für Armaturenbeleuchtung
//Nachricht: Länge 2
//Byte 0: Intensität
//Byte 1: 0x0
//Mögliche Werte:
//Dimmwert    byte[0]
//0           0
//1           28
//2           56
//3           84
//4           112
//5           141
//6           169
//7           197
//8           225
//9           253
DASHBOARD_LIGHTING_ADDR = 0x202,

//Adresse des iDrive Controllers für Buttons & Joystick
IDRIVE_CTRL_BTN_ADDR = 0x267,
//Adresse des iDrive Controllers für Rotation
IDRIVE_CTRL_ROT_ADDR = 0x264,
//Adresse für Status des iDrive Controllers. byte[5] = Counter, byte[6] Initialisiert: 1 = true, 6 = false
IDRIVE_CTRL_STATUS_ADDR = 0x5E7,

//Falls ich mal auf den Trichter komme auf den Touch Controller vom F-Modell zu wechseln...
IDRIVE_CTRL_KEEPALIVE_ADDR_KCAN2 = 0x560,

//Keep-Alive Nachricht, damit der Controller aufgeweckt bleibt
//Quelladresse für Keepalive (ebenfalls KCAN2)
IDRIVE_CTRL_KEEPALIVE_ADDR = 0x501,

//Initialisierung des iDrive Controllers, damit dieser die Drehung übermittelt
//Quelladresse für Rotary-Init
IDRIVE_CTRL_INIT_ADDR = 0x273,
//Nachrichtenadresse erfolgreiche Rotary-Initialisierung
IDRIVE_CTRL_INIT_RESPONSE_ADDR = 0x277,

//Datum und Uhrzeit vom Kombiinstrument (Tacho-Cluster)
KOMBI_DATETIME_ADDR = 0x2F8,

//MFL (Multifunktionslenkrad)
MFL_BUTTON_ADDR = 0x1D6,

//CAS (Car Access System, Schlüsselstatus und Zündung)
CAS_ADDR = 0x130,

//CAS ZV (Zentralverriegelung, Funkschlüssel)
CAS_ZV_ADDR = 0x23A,

//CIC (Car Information Computer)
CIC_ADDR = 0x273,

//RLS (Regen-Licht-Sensorik)
RLS_ADDR = 0x314,

//IHKA (Innenraum Heizung und Klimaanlage) - Status innenraumsensorik
IHKA_ADDR = 0x32E,

//LM (Lichtmodul) Dimmer für Armaturenbeleuchtung bei Abblendlicht AKTIV
LM_DIM_ADDR = 0x202,

//Rückspiegel Abblendfunktion
SPIEGEL_ABBLEND_ADDR = 0x286,

//PDC Abstandsmeldung 1
PDC_ABSTANDSMELDUNG_1_ADDR = 0x1C2,

//PDC Abstandsmeldung 2
PDC_ABSTANDSMELDUNG_2_ADDR = 0x1C3,

//PDC Akustikmeldung
PDC_AKUSTIKMELDUNG_ADDR = 0x1C6,

//LM Status Rückwärtsgang
LM_REVERSESTATUS_ADDR = 0x3B0,

//DME - DDE Powermanagement Batterspiespannung
DMEDDE_POWERMGMT_BATTERY_ADDR = 0x3B4,

//FRM - Indicator Light
FRM_INDICATOR_ADDR = 0x1F6,

//JBE - Drivers Door Status
JBE_DRIVER_DOOR_STATUS_ADDR = 0x0E2,

//JBE - Passenger Door Status
JBE_PASSENGER_DOOR_STATUS_ADDR = 0x0EA,

//CAS - Ignition Status
CAS_IGNITION_STATUS_ADDR = 0x26E,

//Indicator Stalk
INDICATOR_STALK_ADDR = 0x1EE,

//CAS - Door Status
CAS_DOOR_STATUS = 0x2FC,

};



#endif