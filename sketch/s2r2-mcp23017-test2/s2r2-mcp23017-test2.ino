/* ------------------------------------------------- */
/* Suntracker2R2 4. IO Expander MCP23017 Test Sketch */
/* ------------------------------------------------- */
#include <Wire.h>              // Arduino built-in I2C bus function library
#include <Adafruit_MCP23017.h> // https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
Adafruit_MCP23017 mcp3;        // create object for 3rd MCP23017
Adafruit_MCP23017 mcp4;        // create object for 4th MCP23017
uint8_t pin  = 0;              // pin counter to enable LED
  
void setup() {
   Wire.begin();                    // enable I2c bus
   mcp3.begin(0);                   // enable 4th MCP23017 expander chip
   for (pin = 0; pin<16; pin++) {   // run through all 16 pins on the expander
      mcp3.pinMode(pin, OUTPUT);    // set expander pin as output
      mcp3.digitalWrite(pin, HIGH); // set expander pin to '1' (LED ON)
   }
   mcp4.begin(1);                   // enable 4th MCP23017 expander chip
   for (pin = 0; pin<16; pin++) {   // run through all 16 pins on the expander
      mcp4.pinMode(pin, OUTPUT);    // set expander pin as output
      mcp4.digitalWrite(pin, HIGH); // set expander pin to '1' (LED ON)
   }
}

void loop() {}
