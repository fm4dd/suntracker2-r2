/* ------------------------------------------------- */
/* Suntracker2R2 12C Bus Scanner Test Sketch         */
/* ------------------------------------------------- */
#include <Wire.h>      // I2C bus functions library

uint8_t error = 0;     // I2C commuications error
uint8_t address = 0;   // I2C device address
uint8_t counter = 0;   // I2C device counter
  
void setup(){
   Serial.begin(115200);
    while (!Serial);   // wait for serial port to connect.
   Serial.println("Serial OK. Scanning I2C Bus:");
   Wire.begin();       // enable I2C bus, default speed
   for(address = 0; address < 127; address++ ) {
      Serial.print(".");
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0){
         Serial.print("\nDevice found: 0x");
         Serial.println(address,HEX);
         counter++;
         continue;
      }
      if (error == 4) {
         Serial.print("\nDevice error: 0x");
         Serial.println(address,HEX);
         continue;
      }
      delay(100);   
   }
   Serial.print("\n I2C busc scan result: ");
   Serial.print(counter);
   Serial.print(" devices found.");
}

void loop() {}
