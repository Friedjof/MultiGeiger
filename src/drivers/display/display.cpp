// OLED display related code

#include "display.hpp"

#define PIN_DISPLAY_ON 21

#define PIN_OLED_RST 16
#define PIN_OLED_SCL 15
#define PIN_OLED_SDA 4

void DisplayModule::startScreen() {
  char line[20];

  pu8x8->clear();

  if (isLoraBoard) {
    pu8x8->setFont(u8x8_font_amstrad_cpc_extended_f);
    pu8x8->drawString(0, 2, " Multi-");
    pu8x8->drawString(0, 3, " Geiger");
    pu8x8->setFont(u8x8_font_victoriamedium8_r);
    snprintf(line, 9, "%s", VERSION_STR);  // 8 chars + \0 termination
    pu8x8->drawString(0, 4, line);
  } else {
    pu8x8->setFont(u8x8_font_amstrad_cpc_extended_f);
    pu8x8->drawString(0, 0, "  Multi-Geiger");
    pu8x8->setFont(u8x8_font_victoriamedium8_r);
    pu8x8->drawString(0, 1, "________________");
    pu8x8->drawString(0, 3, "Info:boehri.de");
    snprintf(line, 15, "%s", VERSION_STR);  // 14 chars + \0 termination
    pu8x8->drawString(0, 5, line);
  }
  displayIsClear = false;
}

// Legacy free-function API forwarding to a chosen instance.
static DisplayModule gDisplay;
static DisplayModule *gActiveDisplay = &gDisplay;

void DisplayModule::begin(bool loraHardware) {
  gActiveDisplay = this;  // use this instance for legacy wrappers
  isLoraBoard = loraHardware;
  if (isLoraBoard) {
    pu8x8 = &u8x8_lora;
//    pinMode(PIN_DISPLAY_ON, INPUT);
//    digitalWrite(PIN_DISPLAY_ON, LOW);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
  } else {
    pu8x8 = &u8x8;
  }
  pu8x8->begin();
  startScreen();
}

void DisplayModule::clearLine(int line) {
  const char *blanks;
  blanks = isLoraBoard ? "        " : "                ";  // 8 / 16
  pu8x8->drawString(0, line, blanks);
}

void DisplayModule::showStatusLine(const String &txt) {
  if (txt.length() == 0)
    return;
  int line = isLoraBoard ? 5 : 7;
  pu8x8->setFont(u8x8_font_victoriamedium8_r);
  clearLine(line);
  pu8x8->drawString(0, line, txt.c_str());
}

void DisplayModule::setStatus(int index, int value) {
  if ((index >= 0) && (index < STATUS_MAX)) {
    if (status[index] != value) {
      status[index] = value;
      renderStatus();
    }
  } else
    log(ERROR, "invalid parameters: set_status(%d, %d)", index, value);
}

int DisplayModule::getStatus(int index) const {
  return status[index];
}

char DisplayModule::getStatusChar(int index) const {
  if ((index >= 0) && (index < STATUS_MAX)) {
    int idx = status[index];
    if (idx < (int)strlen(status_chars[index]))
      return status_chars[index][idx];
    else
      log(ERROR, "string status_chars[%d] is too short, no char at index %d", index, idx);
  } else
    log(ERROR, "invalid parameters: get_status_char(%d)", index);
  return '?';  // some error happened
}

void DisplayModule::renderStatus(void) {
  char output[17];  // max. 16 chars wide display + \0 terminator
  const char *format = isLoraBoard ? "%c%c%c%c%c%c%c%c" : "%c %c %c %c %c %c %c %c";  // 8 or 16 chars wide
  snprintf(output, 17, format,
           getStatusChar(0), getStatusChar(1), getStatusChar(2), getStatusChar(3),
           getStatusChar(4), getStatusChar(5), getStatusChar(6), getStatusChar(7)
          );
  showStatusLine(output);
}

// Legacy free-function API forwarding to the active instance
void setup_display(bool loraHardware) { gActiveDisplay->begin(loraHardware); }
void display_GMC(unsigned int TimeSec, int RadNSvph, int CPM, bool use_display) { gActiveDisplay->showGmc(TimeSec, RadNSvph, CPM, use_display); }
void clear_displayline(int line) { gActiveDisplay->clearLine(line); }
void display_statusline(String txt) { gActiveDisplay->showStatusLine(txt); }
void set_status(int index, int value) { gActiveDisplay->setStatus(index, value); }
int get_status(int index) { return gActiveDisplay->getStatus(index); }
void display_status(void) { gActiveDisplay->renderStatus(); }
char *format_time(unsigned int secs) {
  static char result[4];
  unsigned int mins = secs / 60;
  unsigned int hours = secs / (60 * 60);
  unsigned int days = secs / (24 * 60 * 60);
  if (secs < 60) {
    snprintf(result, 4, "%2ds", secs);
  } else if (mins < 60) {
    snprintf(result, 4, "%2dm", mins);
  } else if (hours < 24) {
    snprintf(result, 4, "%2dh", hours);
  } else {
    days = days % 100;  // roll over after 100d
    snprintf(result, 4, "%2dd", days);
  }
  return result;
}

void DisplayModule::showGmc(unsigned int TimeSec, int RadNSvph, int CPM, bool use_display) {
  if (!use_display) {
    if (!displayIsClear) {
      pu8x8->clear();
      clearLine(4);
      clearLine(5);
      displayIsClear = true;
    }
    return;
  }

  pu8x8->clear();

  char output[40];
  if (!isLoraBoard) {
    pu8x8->setFont(u8x8_font_7x14_1x2_f);
    sprintf(output, "%3s%7d nSv/h", format_time(TimeSec), RadNSvph);
    pu8x8->drawString(0, 0, output);
    pu8x8->setFont(u8x8_font_inb33_3x6_n);
    sprintf(output, "%5d", CPM);
    pu8x8->drawString(0, 2, output);
  } else {
    pu8x8->setFont(u8x8_font_amstrad_cpc_extended_f);
    sprintf(output, " %7d", RadNSvph);
    pu8x8->drawString(0, 2, output);
    pu8x8->setFont(u8x8_font_px437wyse700b_2x2_f);
    sprintf(output, "%4d", CPM);
    pu8x8->drawString(0, 3, output);
  }
  renderStatus();
  displayIsClear = false;
}
