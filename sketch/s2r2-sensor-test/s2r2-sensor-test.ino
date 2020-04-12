/* ------------------------------------------------- */
/* Suntracker2R2 DS3231 Realtime Clock Test Sketch   */
/* ------------------------------------------------- */
#include <Arduino.h>
#include "uRTCLib.h"              // https://github.com/Naguissa/uRTCLib
#include <U8g2lib.h>              // https://github.com/olikraus/u8g2

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled1(U8G2_R0);
uRTCLib rtc;                     // realtime clock object
char lineStr[15];                // display output buffer, 14-chars + \0

void setup() {
   Wire.begin();                 // enable I2C bus
   rtc.set_rtc_address(0x68);    // Enable realtime clock
   rtc.set_model(URTCLIB_MODEL_DS3231);
   // rtc.set sec,min,hour,dayOfWeek,dayOfMonth,month,year
   // to set the time, enable the line below
   //rtc.set(0, 51, 0, 0, 16, 6, 19);
   oled1.begin();                // enable oled display
   oled1.setFont(u8g2_font_t0_17_mr);
}

void loop() {
   rtc.refresh();                // get new time from clock
   snprintf(lineStr, sizeof(lineStr), "%d/%02d/%02d", rtc.year(),rtc.month(),rtc.day());
   oled1.drawStr(0, 15, lineStr);
   snprintf(lineStr, sizeof(lineStr), "%02d:%02d:%02d", rtc.hour(),rtc.minute(),rtc.second());
   oled1.drawStr(0, 30, lineStr);
   oled1.sendBuffer();          // update oled display
}
