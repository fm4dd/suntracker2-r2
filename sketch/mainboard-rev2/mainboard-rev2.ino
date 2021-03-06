/* ------------------------------------------------- *
 * Suntracker2  mainboard-rev2  June 2019 @ FM4DD    *
 *                                                   * 
 * This code controls Suntracker2 Base-Board v1.6 w. *
 * Displayboard v1.0 for solar azimuth path tracking *
 * Requires suncalc v1.2 generated solar positioning *
 * dataset stored in the root of the MKRZERO MicroSD *
 *                                                   *
 * Always doubleCheck correct sensor HW LSM303D/DLHC *
 * ------------------------------------------------- */
#include <Wire.h>              // Arduino default 12C libary
#include <Arduino.h>           // Arduino default library
#include <SPI.h>               // Arduino default SPI library
#include <SD.h>                // Arduino default SD library
#include "LSM303.h"            // https://github.com/pololu/lsm303-arduino
#include "U8g2lib.h"           // https://github.com/olikraus/u8g2
#include "Adafruit_MCP23017.h" // https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
#include "uRTCLib.h"           // https://github.com/Naguissa/uRTCLib
#include <avr/dtostrf.h>       // dtostrf float conversion

/* ------------------------------------------------- */
/* DEBUG enables debug output to the serial monitor  */
/* ------------------------------------------------- */
//#define DEBUG

/* ------------------------------------------------- */
/* SD card - Arduino MKRZero onboard card reader     */
/* ------------------------------------------------- */
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = SDCARD_SS_PIN;
/* ------------------------------------------------- */
/* Magnetic field Sensor                             */
/* ------------------------------------------------- */
LSM303 compass;
/* ------------------------------------------------- */
/* 2x Oled Display - U8G2_R0 = orientation normal    */
/* ------------------------------------------------- */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled1(U8G2_R0);

/* ------------------------------------------------- */
/* DS3231 Precision RTC clock                        */
/* ------------------------------------------------- */
uRTCLib rtc;
/* ------------------------------------------------- */
/* 4x IO Expanders for a total of 64 I/O ports       */
/* ------------------------------------------------- */
Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;
/* ------------------------------------------------- */
/* Global variable list                              */
/* ------------------------------------------------- */
int8_t hled = 0;             // heading LED (red)
int8_t oldhled = 33;         // 33 > max value 32, indicates "init"
int8_t aled = 0;             // azimuth LED (green)
int8_t oldaled = 33;         // 33 > max value 32, indicates "init"
int8_t rled = 0;             // sunrise LED (red)
int8_t oldrled = 33;         // 33 > max value 32, indicates "init"
int8_t sled = 0;             // sunset LED (red)
int8_t oldsled = 33;         // 33 > max value 32, indicates "init"
uint8_t risehour = 0;        // hour of sunrise
uint8_t risemin = 0;         // minute of sunrise
uint8_t transithour = 0;     // peak altitude hour
uint8_t transitmin = 0;      // peak altitude minute
uint8_t sethour = 0;         // sunset hour
uint8_t setmin = 0;          // sunset minute
float heading;               // North heading
double azimuth;              // sun azimuth angle, north->eastward
double zenith;               // zenith angle, substract from 90 to get altitude
uint16_t riseaz;             // sunrise azimuth, rounded to full degrees
int16_t transalt;            // transit altitude - solar noon peak altitude
uint16_t setaz;              // sunset azimuth, rounded to full degrees
boolean daylight;            // daylight flag, false = night
char lineStr[15];            // display output buffer, 14-chars + \0
char hledStr[5];             // display LED control "-B01"
char dirStr[6];              // display north angle "H263"
char aziStr[6];              // display azimuth angle "A134"
char timeStr[9];             // display time string "HH:MM:SS"
char dateStr[16];            // display date string "YYYY/MM/DD"
char riseStr[16];            // display string "sunrise: 06:45"
char setStr[16];             // display string "sunset:  18:10"
char binfile[13];            // daily sun data file yyyymmdd.bin
char srsfile[13];            // sunrise/sunset file srs-yyyy.bin
const uint8_t dip1  = 0;     // GPIO pin dipswitch-1 on GPIO-0
const uint8_t dip2  = 1;     // GPIO pin dipswitch-2 on GPIO-1
const uint8_t push1 = 2;     // GPIO pin pushbutton-1 on GPIO-2
const uint8_t push2 = 3;     // GPIO pin pushbutton-2 on GPIO-3
const double mdecl = -7.583; // local magnetic declination
uint8_t dippos1 = 0;         // dipswitch 1 position (extra tests)
uint8_t dippos2 = 0;         // dipswitch 2 position (undefined)

/* ------------------------------------------------- */
/* 'arduino logo', 64x32px, XBM format               */
/* ------------------------------------------------- */
static const unsigned char arduLogo[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x0F, 0x00, 0x00, 0xF0, 0x3F, 0x00, 
  0x80, 0xFF, 0x3F, 0x00, 0x00, 0xFE, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 
  0x80, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x01, 0xC0, 0xFF, 0xFF, 0x07, 
  0xF0, 0x07, 0xF8, 0x07, 0xE0, 0x1F, 0xE0, 0x0F, 0xF8, 0x01, 0xE0, 0x0F, 
  0xF0, 0x07, 0x80, 0x0F, 0xFC, 0x00, 0xC0, 0x1F, 0xF8, 0x01, 0x00, 0x1F, 
  0x7C, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x3E, 0x3E, 0x00, 0x00, 0x7E, 
  0x7E, 0xC0, 0x03, 0x3E, 0x3E, 0x00, 0x00, 0x7C, 0x3F, 0xC0, 0x03, 0x3C, 
  0x1E, 0x00, 0x00, 0xF8, 0x1F, 0xC0, 0x03, 0x7C, 0x1E, 0x00, 0x00, 0xF0, 
  0x0F, 0xC0, 0x03, 0x78, 0x1E, 0xFE, 0x1F, 0xF0, 0x07, 0xFC, 0x3F, 0x78, 
  0x1E, 0xFE, 0x1F, 0xE0, 0x03, 0xFC, 0x3F, 0x78, 0x1E, 0xFE, 0x1F, 0xE0, 
  0x07, 0xFC, 0x3F, 0x78, 0x1E, 0xFE, 0x1F, 0xF0, 0x0F, 0xFC, 0x3F, 0x78, 
  0x1E, 0x00, 0x00, 0xF8, 0x0F, 0xC0, 0x03, 0x78, 0x1E, 0x00, 0x00, 0xFC, 
  0x1F, 0xC0, 0x03, 0x3C, 0x3E, 0x00, 0x00, 0x7E, 0x3E, 0xC0, 0x03, 0x3C, 
  0x3C, 0x00, 0x00, 0x3F, 0x7E, 0xC0, 0x03, 0x3E, 0x7C, 0x00, 0x80, 0x1F, 
  0xFC, 0x01, 0x00, 0x1F, 0xF8, 0x00, 0xC0, 0x0F, 0xF8, 0x03, 0x80, 0x1F, 
  0xF8, 0x03, 0xF0, 0x07, 0xF0, 0x0F, 0xC0, 0x0F, 0xF0, 0x0F, 0xFC, 0x03, 
  0xE0, 0x3F, 0xF0, 0x07, 0xE0, 0xFF, 0xFF, 0x01, 0x80, 0xFF, 0xFF, 0x03, 
  0x80, 0xFF, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x00, 0xFF, 0x1F, 0x00, 
  0x00, 0xFC, 0x7F, 0x00, 0x00, 0xF8, 0x03, 0x00, 0x00, 0xE0, 0x0F, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, };

/* ------------------------------------------------- */
/* 'day logo', 32x32px, XBM format                   */
/* ------------------------------------------------- */
static const unsigned char dayImg[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xE0, 0x07, 0x00, 
  0x00, 0xE0, 0x07, 0x00, 0xE0, 0xC0, 0x03, 0x07, 0xF0, 0x81, 0x81, 0x0F, 
  0xF0, 0x01, 0x80, 0x0F, 0xF0, 0x01, 0x80, 0x0F, 0xE0, 0xE0, 0x07, 0x07, 
  0x00, 0xF8, 0x1F, 0x00, 0x00, 0xFC, 0x3F, 0x00, 0x00, 0xFE, 0x7F, 0x00, 
  0x00, 0x3E, 0x7C, 0x00, 0x0C, 0x1F, 0xF8, 0x30, 0x1E, 0x0F, 0xF0, 0x78, 
  0x3E, 0x0F, 0xF0, 0x7C, 0x3E, 0x0F, 0xF0, 0x7C, 0x1E, 0x0F, 0xF0, 0x78, 
  0x0C, 0x1F, 0xF8, 0x30, 0x00, 0x7E, 0x7E, 0x00, 0x00, 0xFE, 0x7F, 0x00, 
  0x00, 0xFC, 0x3F, 0x00, 0x00, 0xF8, 0x1F, 0x00, 0xE0, 0xE0, 0x07, 0x07, 
  0xF0, 0x01, 0x80, 0x0F, 0xF0, 0x01, 0x80, 0x0F, 0xF0, 0x81, 0x81, 0x0F, 
  0xE0, 0xC0, 0x03, 0x07, 0x00, 0xE0, 0x07, 0x00, 0x00, 0xE0, 0x07, 0x00, 
  0x00, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, };

/* ------------------------------------------------- */
/* 'night logo', 32x32px, XBM format                 */
/* ------------------------------------------------- */
static const unsigned char nightImg[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 
  0x80, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xE0, 0x03, 0x00, 0x00, 
  0xE0, 0x07, 0x00, 0x00, 0xF0, 0x07, 0x00, 0x00, 0xF0, 0x07, 0x00, 0x00, 
  0xF0, 0x0F, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0xF8, 0x1F, 0x00, 0x00, 
  0xF0, 0x3F, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00, 
  0xF0, 0xFF, 0x0F, 0x0C, 0xF0, 0xFF, 0xFF, 0x0F, 0xE0, 0xFF, 0xFF, 0x0F, 
  0xE0, 0xFF, 0xFF, 0x07, 0xC0, 0xFF, 0xFF, 0x07, 0x80, 0xFF, 0xFF, 0x03, 
  0x00, 0xFF, 0xFF, 0x01, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xF8, 0x3F, 0x00, 
  0x00, 0xA0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

/* ------------------------------------------------- */
/* 'ghost logo', 32x32px, XBM format                 */
/* ------------------------------------------------- */
static const unsigned char ghostImg[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x00, 
  0x00, 0xFC, 0x1F, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0x00, 
  0x80, 0xFF, 0xFF, 0x00, 0x80, 0xFF, 0xFF, 0x01, 0xC0, 0xFF, 0xFF, 0x01, 
  0xC0, 0xFF, 0xFF, 0x01, 0xC0, 0xC3, 0xC3, 0x03, 0xE0, 0xC3, 0xC3, 0x03, 
  0xE0, 0xC3, 0xC3, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 
  0xE0, 0x3F, 0xFE, 0x03, 0xE0, 0x0F, 0xF8, 0x03, 0xE0, 0x07, 0xF0, 0x03, 
  0xE0, 0x07, 0xF0, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 
  0xE0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x03, 
  0xE0, 0xFF, 0xFF, 0x03, 0xE0, 0xFD, 0xFE, 0x03, 0xE0, 0xFD, 0xBE, 0x03, 
  0xC0, 0x79, 0xBE, 0x03, 0xC0, 0x78, 0x1C, 0x03, 0x80, 0x30, 0x08, 0x02, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

/* ------------------------------------------------- */
/* 'sunrise logo', 16x16px, XBM format               */
/* ------------------------------------------------- */
static const unsigned char riseImg[] U8X8_PROGMEM = {
  0x00, 0x00, 0x80, 0x00, 0x82, 0x40, 0x84, 0x20, 0x08, 0x10, 0xC0, 0x03, 
  0xF0, 0x0F, 0xFA, 0x5F, 0xF8, 0x1F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 
  0x00, 0x00, 0xFC, 0x3F, 0x00, 0x00, 0xE0, 0x07, };

/* ------------------------------------------------- */
/* 'sunset logo', 16x16px, XBM format                */
/* ------------------------------------------------- */
static const unsigned char setImg[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xC0, 0x03, 0xE0, 0x01, 0xF0, 0x00, 
  0x78, 0x00, 0x78, 0x00, 0x78, 0x00, 0xF0, 0x01, 0xE0, 0x0F, 0xFC, 0x3F, 
  0x00, 0x00, 0xFC, 0x3F, 0x00, 0x00, 0xE0, 0x07, };

void setup() {
#ifdef DEBUG
  /* ------------------------------------------------- */
  /* Enable Serial Debug output                        */
  /* ------------------------------------------------- */
  Serial.begin(9600);
  while (!Serial);   // wait for serial port to connect.
  Serial.println("Serial Debug Start");
#endif
  /* ------------------------------------------------- */
  /* Set dipswitch GPIO ports as input                 */
  /* ------------------------------------------------- */
  pinMode(dip1, INPUT);
  pinMode(dip2, INPUT);
  dippos1 = digitalRead(dip1);
  dippos2 = digitalRead(dip2);    
  /* ------------------------------------------------- */
  /* Set pushbutton GPIO ports as input                */
  /* ------------------------------------------------- */
  pinMode(push1, INPUT);
  pinMode(push2, INPUT);
  /* ------------------------------------------------- */
  /* Enable I2C bus                                    */
  /* ------------------------------------------------- */
  Wire.begin();
  /* ------------------------------------------------- */
  /* Enable the OLED graphics display 14char line font */
  /* ------------------------------------------------- */
  oled1.begin();
  oled1.setFont(u8g2_font_t0_17_mr);
  oled1.setFontMode(0);
  oled1.drawXBMP(0,0, 64, 32, arduLogo);
  oled1.drawStr(67, 14, "MKRZERO");
  oled1.drawStr(67, 29, "Init OK");
  oled1.sendBuffer();
  delay(2000);
  /* ------------------------------------------------- */
  /* 1st dip switch enables extended selftests, LOW=ON */
  /* ------------------------------------------------- */
  if(dippos1 == LOW) {
    oled1.drawStr(67, 14, "Modules");
    oled1.drawStr(67, 29, "Testrun");
    /* ------------------------------------------------- */
    /* Identify I2C devices                              */
    /* 0x1D = MM-TXS05 (LSM303D)                         */
    /* 0x20, 0x21, 0x22, 0x23 = MCP23017 1-4             */
    /* 0x3C, 0x3D = SD1306 OLED                          */
    /* 0x68 = DS3231 RTC                                 */
    /* ------------------------------------------------- */
    const int size = 8;
    byte addr[8] = { 0x1D, 0x20, 0x21, 0x22, 0x23, 0x3C, 0x68 };
    int i;
    byte error;
    for(i = 0; i<size; i++ ) {
      /* ------------------------------------------------- */
      /* The i2c_scanner uses the Write.endTransmisstion   */
      /* return value to see if device exists at the addr  */
      /* ------------------------------------------------- */
      snprintf(lineStr, sizeof(lineStr), "I2C check %02x  ", addr[i]);
      oled1.drawStr(0, 47, lineStr);
      oled1.sendBuffer();
      Wire.beginTransmission(addr[i]);
      error = Wire.endTransmission();

      if (error == 0){
        snprintf(lineStr, sizeof(lineStr), "%02x Response OK", addr[i]);
        oled1.drawStr(0, 63, lineStr);
        oled1.sendBuffer();
      }
      else if (error==4) {
        snprintf(lineStr, sizeof(lineStr), "%02x Error      ", addr[i]);
        oled1.drawStr(0, 63, lineStr);
        oled1.sendBuffer();
      }
      else {
        snprintf(lineStr, sizeof(lineStr), "%02x Not Found  ", addr[i]);
        oled1.drawStr(0, 63, lineStr);
        oled1.sendBuffer();
      }
      delay(1000);
    }
    delay(2000);
  } // end dippos != 1, skip extended selftest
  /* ------------------------------------------------- */
  /* Enable the SD card module                         */
  /* ------------------------------------------------- */
  oled1.drawStr(0, 47, "Init SD card: ");
  oled1.sendBuffer();
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    oled1.drawStr(0, 63, "SD card FAIL  ");
    while(1);
  } else {
    switch (card.type()) {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        oled1.drawStr(0, 63, "SD1 card OK   ");
        break;
      case SD_CARD_TYPE_SD2:
        oled1.drawStr(0, 63, "SD2 card OK   ");
        break;
      case SD_CARD_TYPE_SDHC:
        oled1.drawStr(0, 63, "SDHC card OK  ");
        break;
      default:
        oled1.drawStr(0, 63, "Unknown card  ");
    }
  }
  SD.begin(chipSelect); 
  oled1.sendBuffer();
  delay(1500);
  oled1.clearBuffer();
  
  if(dippos1 == LOW) {
    /* ------------------------------------------------- */
    /* Check File read from SD: "dset.txt"               */
    /* ------------------------------------------------- */
 
    File dset = SD.open("DSET.TXT");
    if(dset) {
      oled1.drawStr(0, 14, "Read file dset.txt");
      unsigned long fsize = dset.size();
      snprintf(lineStr, sizeof(lineStr), "Size %ld bytes", fsize);
      oled1.drawStr(0, 30, lineStr);
      oled1.sendBuffer();
      delay(2000);
      while (dset.available()) {
        oled1.drawStr(0, 47, "              ");
        oled1.drawStr(0, 63, "              ");
        oled1.sendBuffer();
        dset.readStringUntil(':').toCharArray(lineStr,15);
        oled1.drawStr(0, 47, lineStr);
        dset.read(lineStr, 1); // skip over one space
        dset.readStringUntil('\n').toCharArray(lineStr,15);
        oled1.drawStr(0, 63, lineStr);
        oled1.sendBuffer();
        delay(1000);
      }
    } else {
      oled1.drawStr(0, 14, "Fail open dset.txt");
      oled1.sendBuffer();
    }
    dset.close();
    delay(2000);
  } // end dippos != 1, skip extended selftest
  oled1.clearBuffer();
  /* ------------------------------------------------- */
  /* Enable the DS3231 RTC clock module                */
  /* ------------------------------------------------- */
  oled1.drawStr(0, 14, "Init DS3231:");
  oled1.sendBuffer();
  rtc.set_rtc_address(0x68);
  rtc.set_model(URTCLIB_MODEL_DS3231);
  rtc.refresh();
  /* ------------------------------------------------- */
  /* To set the initial time, enable the line below    */
  /* ------------------------------------------------- */
  //rtc.set sec,min,hour,dayOfWeek,dayOfMonth,month,year
  //rtc.set(0, 51, 0, 0, 16, 6, 19);
  snprintf(lineStr, sizeof(lineStr), "%d/%02d/%02d %02d:%02d",
  rtc.year(),rtc.month(),rtc.day(),rtc.hour(),rtc.minute());
  oled1.drawStr(0, 30, lineStr);
  oled1.sendBuffer();
  delay(1500);
  oled1.clearBuffer();
  /* ------------------------------------------------- */
  /* Enable the LSM303 Compass sensor module           */
  /* 1. Sunhayato MM-TXS05 SA0 default I2C addr (0x1D) */
  /* compass.init(LSM303::device_D,LSM303::sa0_high);  */
  /* 2. Adafruit LSM303DLHC module (PROD-ID 1120) use: */
  /* compass.init(LSM303::device_DLHC,LSM303::sa0_high);
  /* init() without args tries to determine chip type  */
  /* ------------------------------------------------- */
  boolean mag_found = false;
  oled1.drawStr(0, 14, "Init Compass:");
  oled1.sendBuffer();
  mag_found = compass.init();
  if(mag_found) {
    if(compass.getDeviceType() == 3) oled1.drawStr(0, 30, "SH MM-TXS05");
    else 
      if(compass.getDeviceType() == 2) oled1.drawStr(0, 30, "ADA-LSM303");
    oled1.sendBuffer();
    compass.enableDefault();
    /* ------------------------------------------------- */
    /* LSM303D Calibration values, see Calibrate example */
    /* ------------------------------------------------- */
    compass.m_min = (LSM303::vector<int16_t>){-2461, -2517, -2943};
    compass.m_max = (LSM303::vector<int16_t>){+1762, +2254, +1969};
    oled1.drawStr(100, 30, "OK");
    oled1.sendBuffer();
    /* ------------------------------------------------- */
    /* Aquire compass sensor data                        */
    /* ------------------------------------------------- */
    compass.read();
    if(compass.getDeviceType() == 3) heading = compass.heading() + mdecl;
    if(compass.getDeviceType() == 2) heading = compass.heading((LSM303::vector<int>){1,0,0}) + mdecl;
    if(heading < 0) heading = 360.0 + heading;
    /* ------------------------------------------------- */
    /* print North heading to TFT                        */
    /* ------------------------------------------------- */
    dtostrf(heading,6,4,lineStr);
    oled1.drawStr(0,46, "Heading: ");
    oled1.drawStr(0,62, lineStr);
    oled1.sendBuffer();
  }
  else {
    oled1.drawStr(0, 30, "LSM303 Not Found");
  }
  delay(2000);
  oled1.clearBuffer();
  /* ------------------------------------------------- */
  /* Enable the IO port expansion modules. I2C address */
  /* is given to begin() as parameter per A-pin values */
  /* 0 = 0x20, 1 = 0x21, 2 = 0x22, 3 = 0x23, ... max 7 */
  /* ------------------------------------------------- */
  oled1.drawStr(0, 14, "Init MCP23017:");
  oled1.sendBuffer();
  mcp1.begin(0);     /* Enable the MCP23017 module 1 */
  mcp2.begin(1);     /* Enable the MCP23017 module 2 */
  mcp3.begin(2);     /* Enable the MCP23017 module 3 */
  mcp4.begin(3);     /* Enable the MCP23017 module 4 */
  /* Configure all MCP23017 ports in LED output mode */
  for(int i=0; i<16; i++) {
    mcp1.pinMode(i, OUTPUT);
    mcp2.pinMode(i, OUTPUT);
    mcp3.pinMode(i, OUTPUT);
    mcp4.pinMode(i, OUTPUT);
    mcp1.digitalWrite(i, LOW);
    mcp2.digitalWrite(i, LOW);
    mcp3.digitalWrite(i, LOW);
    mcp4.digitalWrite(i, LOW);
  }
  oled1.drawStr(0, 30, "4xMCP23017 OK");
  oled1.sendBuffer();
  delay(500);
  
  if(dippos1 == LOW) {
    led_lightcheck();
    /* ----------------------------------------------- */
    /* Single LED walkthrough                          */
    /* ----------------------------------------------- */
    stepled_red(oled1, 20);
    stepled_green(oled1, 20);
    stepled_red(oled1, 20);
    stepled_green(oled1, 20);
  }  
  lightshow(60);
  /* ------------------------------------------------- */
  /* Read the solar position from file                 */
  /* ------------------------------------------------- */
  snprintf(binfile, sizeof(binfile), "20%d%02d%02d.BIN", 
             rtc.year(),rtc.month(),rtc.day());
  solar_position(binfile);
  /* ------------------------------------------------- */
  /* Read sunrise sunset time from file                */
  /* ------------------------------------------------- */
  snprintf(srsfile, sizeof(srsfile), "SRS-20%d.BIN", rtc.year());
  get_suntime(srsfile);
  snprintf(riseStr, sizeof(riseStr), "rise %02d:%02d %03d",
           risehour,risemin, riseaz);
  snprintf(setStr, sizeof(setStr), "set  %02d:%02d %03d",
           sethour, setmin, setaz);
  delay(1000);
  oled1.clearBuffer();
} // end setup

void loop() {
  /* ------------------------------------------------- */
  /* Aquire time                                       */
  /* Every new minute read solar position data         */
  /* ------------------------------------------------- */
  rtc.refresh();
  if(rtc.second() == 0) {
    if(rtc.minute() == 0) {
      snprintf(binfile, sizeof(binfile), "20%d%02d%02d.BIN", 
                rtc.year(),rtc.month(),rtc.day());
      if(rtc.day() == 1) {
        snprintf(srsfile, sizeof(srsfile), "SRS-20%d.BIN", 
                rtc.year());
      }
      get_suntime(srsfile);
      snprintf(riseStr, sizeof(riseStr), "rise %02d:%02d %03d",
               risehour,risemin, riseaz);
      snprintf(setStr, sizeof(setStr), "set  %02d:%02d %03d",
               sethour, setmin, setaz);
    }
    solar_position(binfile);
    display_daysymbol(oled1);
  }
  /* ------------------------------------------------- */
  /* Show time on OLED                                 */
  /* ------------------------------------------------- */
  snprintf(dateStr, sizeof(dateStr), "20%d/%02d/%02d",
  rtc.year(),rtc.month(),rtc.day());
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
  rtc.hour(),rtc.minute(),rtc.second());
  oled1.setFont(u8g2_font_t0_17_mr);
  oled1.setCursor(0, 47);
  oled1.print(dateStr);
  oled1.setCursor(0, 63);
  oled1.print(timeStr);

  /* ------------------------------------------------- */
  /* Aquire compass sensor data                        */
  /* ------------------------------------------------- */
  compass.read();
  
  /* ------------------------------------------------- */
  /* If the sensor board orientation is unaligned, set */
  /* compass.heading((LSM303::vector<int>){0,1,0}) for */
  /* pointing along the Y-axis for example. Remove if  */
  /* the X-axis is the point of reference on a LSM303D */
  /* ------------------------------------------------- */
  /* Because magnetic north != real north, we adjust:  */
  /* Calculate magnetic declination and convergence    */
  /* to determine true North from magnetic North       */ 
  /* https://www.ngdc.noaa.gov/geomag/WMM/soft.shtml   */
  /* ------------------------------------------------- */
  if(compass.getDeviceType() == 3) heading = compass.heading() + mdecl;
  if(compass.getDeviceType() == 2) heading = compass.heading((LSM303::vector<int>){1,0,0}) + mdecl;
  
  /* ------------------------------------------------- */
  /* print North heading to OLED                       */
  /* ------------------------------------------------- */
  snprintf(dirStr, sizeof(dirStr), "H%03d", (int)round((heading) * 10 / 10.0));
  oled1.setFont(u8g2_font_inr19_mr);
  oled1.drawStr(0,26, dirStr);

  /* ------------------------------------------------- */
  /* Convert heading angle to LED range value (0..32)  */   
  /* ------------------------------------------------- */
  hled = (uint8_t)round((heading / 11.25) * 10 / 10.0);
  aled = (uint8_t)round((azimuth / 11.25) * 10 / 10.0);
  rled = (uint8_t)round((riseaz / 11.25) * 10 / 10.0);
  sled = (uint8_t)round((setaz / 11.25) * 10 / 10.0);
  /* ------------------------------------------------- */
  /* If 32 is returned, it should roll over to 0       */   
  /* ------------------------------------------------- */
  if(hled > 31) hled = 0;
  if(aled > 31) aled = 0;
  if(rled > 31) rled = 0;
  if(sled > 31) sled = 0;
  /* ------------------------------------------------- */
  /* Because LED-1 is 90 degrees offset, we compensate */
  /* ------------------------------------------------- */
  hled = hled + 24; if(hled > 31) hled = hled - 32;
  /* ------------------------------------------------- */
  /* Reverse counting to keep the LED aligned to North */
  /* ------------------------------------------------- */
  hled = 32 - hled; if(hled > 31) hled = 0;
  /* ------------------------------------------------- */
  /* Remaining LED follow heading                      */
  /* ------------------------------------------------- */
  aled = hled + aled; if(aled > 31) aled = aled - 32;
  rled = hled + rled; if(rled > 31) rled = rled - 32;
  sled = hled + sled; if(sled > 31) sled = sled - 32;

#ifdef DEBUG
   var2serial();
#endif 

  /* ------------------------------------------------- */
  /* Set heading LED                                   */
  /* ------------------------------------------------- */
  if (hled != oldhled) {
    if(oldhled < 16) mcp1.digitalWrite(oldhled, LOW);
    else if(oldhled < 32) mcp2.digitalWrite((oldhled-16), LOW);
    if(hled < 16) mcp1.digitalWrite(hled, HIGH);
    if(hled > 15) mcp2.digitalWrite((hled-16), HIGH);
    display_hdgled(oled1, oldhled, hled);
    oldhled = hled;
  }
  
  /* ------------------------------------------------- */
  /* Set azimuth LED                                   */
  /* ------------------------------------------------- */
  if (aled != oldaled) {
    if(oldaled < 16) mcp3.digitalWrite(oldaled, LOW);
    else if(oldaled < 32) mcp4.digitalWrite((oldaled-16), LOW);
    if(aled < 16) mcp3.digitalWrite(aled, HIGH);
    if(aled > 15) mcp4.digitalWrite((aled-16), HIGH);
    oldaled = aled;
  }

  /* ------------------------------------------------- */
  /* Set sunrise LED                                   */
  /* ------------------------------------------------- */
  if (rled != oldrled) {
    if(oldrled < 16) mcp1.digitalWrite(oldrled, LOW);
    else if(oldrled < 32) mcp2.digitalWrite((oldrled-16), LOW);
    if(rled < 16) mcp1.digitalWrite(rled, HIGH);
    if(rled > 15) mcp2.digitalWrite((rled-16), HIGH);
    oldrled = rled;
  }
  
  /* ------------------------------------------------- */
  /* Set sunset LED                                   */
  /* ------------------------------------------------- */
  if (sled != oldsled) {
    if(oldsled < 16) mcp1.digitalWrite(oldsled, LOW);
    else if(oldsled < 32) mcp2.digitalWrite((oldsled-16), LOW);
    if(sled < 16) mcp1.digitalWrite(sled, HIGH);
    if(sled > 15) mcp2.digitalWrite((sled-16), HIGH);
    oldsled = sled;
  }

  oled1.updateDisplay();
  delay(50);
}

/* ------------------------------------------------- */
/* get solar postion record for current time.        */
/* ------------------------------------------------- */
void solar_position(char *file) {
  uint8_t linebuf[19];
  File bin = SD.open(file);
  while (bin.available()) {
    bin.read(linebuf, sizeof(linebuf));
    if(linebuf[0] == rtc.hour() && linebuf[1] == rtc.minute()) {
      memcpy(&azimuth, &linebuf[3], sizeof(double));
      memcpy(&zenith, &linebuf[11], sizeof(double));
      if(linebuf[2] == 1) daylight = 1;
      else daylight = 0;
      break;
    }
  }
  bin.close();
}

/* ------------------------------------------------- */
/* get sunrise/transit/sunset record for current day */
/* ------------------------------------------------- */
void get_suntime(char *file) {
  uint8_t linebuf[14];
  File bin = SD.open(file);
  while (bin.available()) {
    bin.read(linebuf, sizeof(linebuf));
    if(linebuf[0] == rtc.month() && linebuf[1] == rtc.day()) {
      risehour=linebuf[2];
      risemin=linebuf[3];
      memcpy(&riseaz, &linebuf[4], sizeof(uint16_t));
      transithour=linebuf[6];
      transitmin=linebuf[7];
      memcpy(&transalt, &linebuf[8], sizeof(int16_t));
      sethour=linebuf[10];
      setmin=linebuf[11];
      memcpy(&setaz, &linebuf[12], sizeof(uint16_t));
      break;
    }
  }
  bin.close();
}

/* ------------------------------------------------- */
/* led_lightcheck() LED on/off tests for all 4 units */
/* ------------------------------------------------- */
void led_lightcheck() {
  /* ------------------------------------------------- */
  /* Blink all ports. 0xFFFF lights both GPIOA and B.  */
  /* Set 0xFF only lights GPIOA, 0x00FF lights only B  */
  /* ------------------------------------------------- */
  mcp1.writeGPIOAB(0xFFFF);
  delay(500);
  mcp1.writeGPIOAB(0x00);
  mcp2.writeGPIOAB(0xFFFF);
  mcp3.writeGPIOAB(0xFFFF);
  delay(500);
  mcp2.writeGPIOAB(0x00);
  mcp3.writeGPIOAB(0x00);
  mcp4.writeGPIOAB(0xFFFF);
  delay(500);
  mcp4.writeGPIOAB(0x00);
}

/* ------------------------------------------------- */
/* Show azimuth LED control data on OLED             */
/* ------------------------------------------------- */
void display_aziled(U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled, int old, int now) {
  char aledStr[5];
  if(old < 16)
    snprintf(aledStr, sizeof(aledStr), "-C%02d", old);
  else if(old < 32)
    snprintf(aledStr, sizeof(aledStr), "-D%02d", old-16);
  else snprintf(aledStr, sizeof(aledStr), "init");
  oled.setFont(u8g2_font_t0_17_mr);
  oled.drawStr(82,14, aledStr);
  if(now < 16)
    snprintf(aledStr, sizeof(aledStr), "+C%02d", now);
  if(now > 15)
    snprintf(aledStr, sizeof(aledStr), "+D%02d", (now-16));
  oled.drawStr(82,30, aledStr);
  oled.sendBuffer();
}
  /* ------------------------------------------------- */
  /* Show north heading LED control data on OLED       */
  /* ------------------------------------------------- */
void display_hdgled(U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled, int old, int now) {
  char hledStr[5];
  if(old < 16)
    snprintf(hledStr, sizeof(hledStr), "-A%02d", old);
  else if(old < 32)
    snprintf(hledStr, sizeof(hledStr), "-B%02d", old-16);
  else snprintf(hledStr, sizeof(hledStr), "init");
  oled.setFont(u8g2_font_t0_17_mr);
  oled.drawStr(82,14, hledStr);
  if(now < 16)
    snprintf(hledStr, sizeof(hledStr), "+A%02d", now);
  if(now > 15)
    snprintf(hledStr, sizeof(hledStr), "+B%02d", (now-16));
  oled.drawStr(82,30, hledStr);
  oled.sendBuffer();
}

/* ------------------------------------------------- */
/* Single LED walkthrough - Red: mcp1,2, port A,B    */
/* ------------------------------------------------- */
void stepled_red(U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled, int millisec) {
  oled.setFont(u8g2_font_t0_17_mr);
  oled.setFontMode(0);
  char aledStr[5];
  mcp1.writeGPIOAB(0x00);
  mcp2.writeGPIOAB(0x00);
  oled.clearBuffer();
  oled.drawStr(0, 14, "MCP23017-A:");
  oled.drawStr(0, 30, "Port:");
  oled.sendBuffer();
  for(int i=0; i<16; i++) {
    mcp1.digitalWrite(i, HIGH);
    snprintf(aledStr, sizeof(aledStr), "+A%02d", i);
    oled.drawStr(48, 30, aledStr);
    if(i>0){
      mcp1.digitalWrite(i-1, LOW);
      snprintf(aledStr, sizeof(aledStr), "-A%02d", i-1);
      oled.drawStr(88, 30, aledStr);
    }
    oled.updateDisplay();
    delay(millisec);
  }
  mcp1.digitalWrite(15, LOW);
  oled.drawStr(0, 30, "Port A done   ");
  oled.sendBuffer();
  oled.drawStr(0, 47, "MCP23017-B:");
  oled.drawStr(0, 63, "Port:");
  oled.sendBuffer();
  for(int i=0; i<16; i++) {
    mcp2.digitalWrite(i, HIGH);
    snprintf(aledStr, sizeof(aledStr), "+B%02d", i);
    oled.drawStr(48, 63, aledStr);
    if(i>0){
      mcp2.digitalWrite(i-1, LOW);
      snprintf(aledStr, sizeof(aledStr), "-B%02d", i-1);
      oled.drawStr(88, 63, aledStr);
    }
    oled.sendBuffer();
    delay(millisec);
  }
  mcp2.digitalWrite(15, LOW);
  oled.drawStr(0, 63, "Port B done   ");
  oled.sendBuffer();
}

/* ------------------------------------------------- */
/* Single LED walkthrough - Green: mcp3,4, port C,D  */
/* ------------------------------------------------- */
void stepled_green(U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled, int millisec) {
  oled.setFont(u8g2_font_t0_17_mr);
  oled.setFontMode(0);
  char aledStr[5];
  mcp3.writeGPIOAB(0x00);
  mcp4.writeGPIOAB(0x00);
  oled.clearBuffer();
  oled.drawStr(0, 14, "MCP23017-C:");
  oled.drawStr(0, 30, "Port:");
  oled.updateDisplay();
  for(int i=0; i<16; i++) {
    mcp3.digitalWrite(i, HIGH);
    snprintf(aledStr, sizeof(aledStr), "+C%02d", i);
    oled.drawStr(48, 30, aledStr);
    if(i>0){
      mcp3.digitalWrite(i-1, LOW);
      snprintf(aledStr, sizeof(aledStr), "-C%02d", i-1);
      oled.drawStr(88, 30, aledStr);
    }
    oled.updateDisplay();
    delay(millisec);
  }
  mcp3.digitalWrite(15, LOW);
  oled.drawStr(0, 30, "Port C done   ");
  oled.sendBuffer();
  oled.drawStr(0, 47, "MCP23017-D:");
  oled.drawStr(0, 63, "Port:");
  oled.updateDisplay();
  for(int i=0; i<16; i++) {
    mcp4.digitalWrite(i, HIGH);
    snprintf(aledStr, sizeof(aledStr), "+D%02d", i);
    oled.drawStr(48, 63, aledStr);
    if(i>0){
      mcp4.digitalWrite(i-1, LOW);
      snprintf(aledStr, sizeof(aledStr), "-D%02d", i-1);
      oled.drawStr(88, 63, aledStr);
    }
    oled.sendBuffer();
    delay(millisec);
  }
  mcp4.digitalWrite(15, LOW);
  oled.drawStr(0, 63, "Port D done   ");
  oled.updateDisplay();
}

void display_daysymbol(U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled) {
  oled.setFont(u8g2_font_t0_17_mr);
  if(daylight) oled.drawXBMP(96,32, 32, 32, dayImg);
  else if (rtc.hour() == 23) oled.drawXBMP(96,32, 32, 32, ghostImg);
  else oled.drawXBMP(96,32, 32, 32, nightImg);
  oled.sendBuffer();
}

/* ------------------------------------------------- */
/* Lightshow demonstration on a 32x 2-color LED ring */
/* ------------------------------------------------- */
void lightshow(int millisec) {
  /* light up 16 and remove one led on either side until all off */
  int decline[9] = { 0xFFFF, 0x7FFE, 0x3FFC, 0x1FF8, 0xFF0, 0x7E0, 0x3C0, 0x180, 0x0 };
  /* switch on every second led and alternate with its neighbor */
  int alternate[2] = { 0xAAAA, 0x5555 };
  /* light up 1-4 leds, pattern 1-12-123-1234 for one 16-LED set */
  int null2four[5] = { 0x0, 0x8888, 0xCCCC, 0xEEEE, 0xFFFF };
  /* light up 1 led and move it, pattern 1-5-9-13, 2-6-10-14, 3-7-11-15, 4-8-12-16 */
  int move4[4] = { 0x8888, 0x4444, 0x2222, 0x1111 };
  /* light up all led and move one dark led from right 2 left: 
   * start at 0xFFFF, loop subtract, stop at 0x7FFF */
 int move1off[17] = { 0xFFFF, 0xFFFE, 0xFFFD, 0xFFFB, 0xFFF7, 0xFFEF, 0xFFDF, 0xFFBF,
                      0xFF7F, 0xFEFF, 0xFDFF, 0xFBFF, 0xF7FF, 0xEFFF, 0xDFFF, 0xBFFF, 0x7FFF };
  /* To move alternate color in the dark spot, right 2 left: */
  int move1on[16] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100,
                    0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };
  
   for(uint8_t i = 0; i < 9; i++) {
     mcp1.writeGPIOAB(decline[i]);
     mcp2.writeGPIOAB(decline[i]);
     delay(millisec);
   }
   for(uint8_t i = 8; i <= 0; i--) {
     mcp1.writeGPIOAB(decline[i]);
     mcp2.writeGPIOAB(decline[i]);
     delay(millisec);
   }
   for(uint8_t i = 0; i < 9; i++) {
     mcp1.writeGPIOAB(decline[i]);
     mcp2.writeGPIOAB(decline[i]);
     delay(millisec);
   }
   delay(millisec * 4);
   for(uint8_t i = 0; i < 9; i++) {
     mcp3.writeGPIOAB(decline[i]);
     mcp4.writeGPIOAB(decline[i]);
     delay(millisec);
   }
   for(uint8_t i = 8; i <= 0; i--) {
     mcp3.writeGPIOAB(decline[i]);
     mcp4.writeGPIOAB(decline[i]);
     delay(millisec);
   }
   for(uint8_t i = 0; i < 9; i++) {
     mcp3.writeGPIOAB(decline[i]);
     mcp4.writeGPIOAB(decline[i]);
     delay(millisec);
   }
  delay(millisec * 4);
  for(uint8_t j = 0; j < 8; j++) {
    for(uint8_t i = 0; i < 5; i++) {
      mcp3.writeGPIOAB(null2four[i]);
      mcp4.writeGPIOAB(null2four[i]);
      delay(millisec);
    }
    for(uint8_t i = 5; i = 0; i--) {
      mcp3.writeGPIOAB(null2four[i]);
      mcp4.writeGPIOAB(null2four[i]);
      delay(millisec);
    }
  }
  mcp1.writeGPIOAB(0x00);
  mcp2.writeGPIOAB(0x00);
  mcp3.writeGPIOAB(0x00);
  mcp4.writeGPIOAB(0x00);
  for(uint8_t j = 0; j < 8; j++) {
      for(uint8_t i = 0; i < 5; i++) {
      mcp1.writeGPIOAB(move4[i]);
      mcp2.writeGPIOAB(move4[i]);
      delay(millisec);
    }
  }
  mcp1.writeGPIOAB(0x00);
  mcp2.writeGPIOAB(0x00);
}

#ifdef DEBUG
/* ------------------------------------------------- */
/* Debug LED and motor values to serial              */
/* ------------------------------------------------- */
void var2serial() {
  Serial.print("Heading LED: ");
  Serial.print(hled);
  Serial.print(" old: ");
  Serial.print(oldhled);
  Serial.print(" Azimuth LED: ");
  Serial.print(aled);
  Serial.print(" old: ");
  Serial.print(oldaled);
  Serial.print(" M1 target: ");
  Serial.print(m1tpos);
  Serial.print(" M1 current: ");
  Serial.print(m1cpos);
  Serial.print(" Push-1: ");
  Serial.print(digitalRead(push1));
  Serial.print(" Push-2: ");
  Serial.println(digitalRead(push2));
}
/* ------------------------------------------------- */
/* Debug DIP switch and button values to serial      */
/* ------------------------------------------------- */
void swi2serial() {
  Serial.print("DIP-1: ");
  Serial.print(dippos1);
  Serial.print(" DIP-2: ");
  Serial.print(dippos2);
  Serial.print(" Push-1: ");
  Serial.print(digitalRead(push1));
  Serial.print(" Push-2: ");
  Serial.println(digitalRead(push2));
}
#endif
