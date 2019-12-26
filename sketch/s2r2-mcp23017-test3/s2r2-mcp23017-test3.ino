/* ------------------------------------------------- */
/* Suntracker2R2 Wandering LED Test Sketch           */
/* ------------------------------------------------- */
#include <Wire.h>              // Arduino built-in I2C bus function library
#include <Adafruit_MCP23017.h> // https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library

Adafruit_MCP23017 mcp1;        // create object for 1st MCP23017
Adafruit_MCP23017 mcp2;        // create object for 2nd MCP23017
uint8_t pin  = 0;              // pin counter to enable LED
uint8_t led  = 0;              // led counter

  
void setup() {
   Wire.begin();                    // enable I2c bus
   mcp1.begin(0);                   // enable 1st MCP23017 expander chip
   mcp2.begin(1);                   // enable 2nd MCP23017 expander chip

   for (pin=0; pin<16; pin++) {     // run through all 16 pins on the expander
      mcp1.pinMode(pin, OUTPUT);    // set expander pin as output
      mcp1.digitalWrite(pin, LOW);  // set expander pin to '0' (LED OFF)
      mcp2.pinMode(pin, OUTPUT);    // set expander pin as output
      mcp2.digitalWrite(pin, LOW);  // set expander pin to '0' (LED OFF)
   }
}

void loop() {
   for (led=0; led<32; led++) {         // run through all 32 led
      if(led < 16) {                    // for led 0-15, work on mcp1
      pin = led;                        // pin number equals led
      mcp1.digitalWrite(pin, HIGH);     // set expander pin to '1' (LED ON)
      if(pin == 0)                      // if we are on pin 0
         mcp2.digitalWrite(15, LOW);    // turn off the last LED on mcp2
      else
         mcp1.digitalWrite(pin-1, LOW); // turn off the previous LED
      }
      else {                            // we are on led 16 or above
         pin = led-16;                  // pin number equals led minus 16
         mcp2.digitalWrite(pin, HIGH);  // set expander pin to '1' (LED ON)
      if(pin == 0)                      // if we are on pin 0
         mcp1.digitalWrite(15, LOW);    // turn off the last LED on mcp1
      else
         mcp2.digitalWrite(pin-1, LOW); // turn off the previous LED
      }
      delay(60);
   }
}
