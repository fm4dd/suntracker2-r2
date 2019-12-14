/* ------------------------------------------------- */
/* Suntracker2R2 DIP and Pushbutton Test Sketch      */
/* ------------------------------------------------- */
#include "Arduino.h"   // default library

#define DIP1 0         // dip switch 1 <-> IO-pin 0
#define DIP2 1         // dip switch 2 <-> IO-pin 1
#define KEY1 2         // pushbutton 1 <-> IO-pin 2
#define KEY2 3         // pushbutton 2 <-> IO-pin 3

uint8_t dippos1 = 1;   // dip switch 1 position
uint8_t dippos2 = 1;   // dip switch 2 position
uint8_t push1 = 1;     // pushbutton 1 position
uint8_t push2 = 1;     // pushbutton 2 position

void setup() {
   Serial.begin(115200);
    while (!Serial);   // wait for serial port to connect.
   Serial.println("Serial OK");

   /* ------------------------------------------------- */
   /* Set dip switch and push button IO pins for input  */
   /* ------------------------------------------------- */
   pinMode(DIP1, INPUT);
   pinMode(DIP2, INPUT);
   pinMode(KEY1, INPUT);
   pinMode(KEY2, INPUT);

   pinMode(push1, INPUT);
   pinMode(push2, INPUT);
   Serial.println("DIP and KEY pins set for input.");
}

void loop() {
   /* ------------------------------------------------- */
   /* Get and display dip switch and pushbutton status  */
   /* ------------------------------------------------- */
   Serial.print(" DIP1: "); dippos1 = digitalRead(DIP1); Serial.print(dippos1);
   Serial.print(" DIP2: "); dippos2 = digitalRead(DIP2); Serial.print(dippos2);
   Serial.print(" KEY1: ");   push1 = digitalRead(KEY1); Serial.print(push1);
   Serial.print(" KEY1: ");   push2 = digitalRead(KEY2); Serial.print(push2);
   Serial.println();
   delay(500);
}
