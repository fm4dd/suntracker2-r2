/* ------------------------------------------------- */
/* Suntracker2R2 IO Expander MCP23017 Test Sketch    */
/* ------------------------------------------------- */
#include <Wire.h>              // Arduino built-in I2C bus function library
#include <Adafruit_MCP23017.h> // https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library

Adafruit_MCP23017 mcp1;        // create object for 1st MCP23017
  
void setup() {
   Serial.begin(115200);
   while (!Serial);            // wait for serial port to connect.
   Serial.println("Serial OK");
   Wire.begin();               // enable I2c bus
   mcp1.begin();               // enable 1st MCP23017 expander chip
   mcp1.pinMode(0, OUTPUT);    // set 1st expander pin-0 as output
   mcp1.digitalWrite(0, LOW);  // set 1st expander pin-0 to '0'
}

void loop() {
   mcp1.digitalWrite(0, HIGH); // set 1st expander pin-0 to '1'
   Serial.print("1st MCP23017 pin-0 state: ");
   Serial.print(mcp1.digitalRead(0));
   delay(500);
   
   mcp1.digitalWrite(0, LOW);  // set 1st expander pin-0 to '0'
   Serial.print(" state: ");
   Serial.println(mcp1.digitalRead(0));
   delay(500);
}
