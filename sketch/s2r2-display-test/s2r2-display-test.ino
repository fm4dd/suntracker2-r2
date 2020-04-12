/* ------------------------------------------------- */
/* Suntracker2R2 4SSD1306 OLED Display Test Sketch   */
/* ------------------------------------------------- */
#include <Arduino.h>
#include <U8g2lib.h>           // https://github.com/olikraus/u8g2

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled1(U8G2_R0);
  
void setup() {
   oled1.begin();
   oled1.setFont(u8g2_font_t0_17_mr);
   oled1.drawStr(0, 14, "Hello World");
   oled1.sendBuffer();
}

void loop(void) {}
