//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>

#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "LedCtrl"



// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 92 bytes
  { 255,6,0,0,0,85,0,16,26,1,6,0,1,1,61,61,2,26,4,128,
  1,63,60,5,2,0,3,3,1,70,8,22,2,26,129,0,9,71,18,6,
  17,83,111,108,105,100,0,129,0,9,78,18,6,17,67,121,108,111,110,0,
  129,0,9,85,18,6,17,82,97,105,110,98,111,119,0,2,1,1,94,61,
  5,2,27,31,31,79,78,0,79,70,70,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t userColor_r; // =0..255 Red color value 
  uint8_t userColor_g; // =0..255 Green color value 
  uint8_t userColor_b; // =0..255 Blue color value 
  int8_t brightnessSlider; // =0..100 slider position 
  uint8_t effectSelect; // =0 if select position A, =1 if position B, =2 if position C, ... 
  uint8_t LedsEnabled; // =1 if switch ON and =0 if OFF 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////