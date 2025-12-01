// OLED display related code

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Arduino.h>
#include <U8x8lib.h>

#include "core/core.hpp"
#include "config/config.hpp"

// supported status indexes and values:

// these are used for all subsystems
#define ST_NODISPLAY 0
#define ST_OFF 0
#define ST_OK 1
#define ST_ERROR 2

#define STATUS_WIFI 0
#define ST_WIFI_OFF 0
#define ST_WIFI_CONNECTED 1
#define ST_WIFI_ERROR 2
#define ST_WIFI_CONNECTING 3
#define ST_WIFI_AP 4

#define STATUS_SCOMM 1
#define ST_SCOMM_OFF 0
#define ST_SCOMM_IDLE 1
#define ST_SCOMM_ERROR 2
#define ST_SCOMM_SENDING 3
#define ST_SCOMM_INIT 4

#define STATUS_MADAVI 2
#define ST_MADAVI_OFF 0
#define ST_MADAVI_IDLE 1
#define ST_MADAVI_ERROR 2
#define ST_MADAVI_SENDING 3
#define ST_MADAVI_INIT 4

#define STATUS_TTN 3
#define ST_TTN_OFF 0
#define ST_TTN_IDLE 1
#define ST_TTN_ERROR 2
#define ST_TTN_SENDING 3
#define ST_TTN_INIT 4

#define STATUS_BLE 4
#define ST_BLE_OFF 0
#define ST_BLE_CONNECTED 1
#define ST_BLE_ERROR 2
#define ST_BLE_CONNECTABLE 3
#define ST_BLE_INIT 4

// status index 5 is still free

// status index 6 is still free

#define STATUS_HV 7
#define ST_HV_OK 1
#define ST_HV_ERROR 2

#define STATUS_MAX 8

void set_status(int index, int value);
int get_status(int index);
void display_status(void);

// Thin OO wrapper for display operations.
class DisplayModule {
public:
  void begin(bool loraHardware);
  void showGmc(unsigned int timeSec, int radNSvph, int cpm, bool useDisplay);
  void applyDisplaySetting(bool useDisplay);
  void clearLine(int line);
  void showStatusLine(const String &txt);
  void setStatus(int index, int value);
  int getStatus(int index) const;
  void renderStatus();

private:
  void startScreen();
  char getStatusChar(int index) const;

  U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8{PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA};
  U8X8_SSD1306_64X32_NONAME_HW_I2C u8x8_lora{PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA};
  U8X8 *pu8x8 = nullptr;
  bool displayIsClear = false;
  bool isLoraBoard = false;
  int status[STATUS_MAX] = {ST_NODISPLAY, ST_NODISPLAY, ST_NODISPLAY, ST_NODISPLAY,
                            ST_NODISPLAY, ST_NODISPLAY, ST_NODISPLAY, ST_NODISPLAY};
  const char *status_chars[STATUS_MAX] = {
    ".W0wA",  // WiFi
    ".s1S?",  // sensor.community
    ".m2M?",  // madavi
    ".t3T?",  // TTN
    ".B4b?",  // BLE
    ".",      // free
    ".",      // free
    ".H7",    // HV
  };
};

// Legacy free functions (forward to singleton)
void setup_display(bool loraHardware);
void display_GMC(unsigned int TimeSec, int RadNSvph, int CPM, bool use_display);
void clear_displayline(int line);
void display_statusline(String txt);
void set_status(int index, int value);
int get_status(int index);
void display_status(void);

#endif // _DISPLAY_H_
