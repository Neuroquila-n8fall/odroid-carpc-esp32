#include <BleKeyboard.h>
#include <ble_keyboard.h>

BleKeyboard bleKeyboard("IDRIVE CTRL", "FNTX", 100);
const MediaKeyReport KEY_MEDIA_TRACK_FASTFORWARD = {0,16};
const MediaKeyReport KEY_MEDIA_TRACK_REWIND = {0,32};