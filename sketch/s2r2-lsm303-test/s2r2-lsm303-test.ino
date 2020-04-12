/* ------------------------------------------------- */
/* Suntracker2R2 LSM303 Magnetometer Test Sketch     */
/* ------------------------------------------------- */
#include <Wire.h>
#include <LSM303.h>

LSM303 compass;

void setup() {
   Serial.begin(115200);
   while (!Serial);   // wait for serial port to connect.
   Serial.println("Serial OK");

   Wire.begin();
   compass.init();
   compass.enableDefault();
   /* Default Calibration values of +/-32767 for each axis */
   compass.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
   compass.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};
}

void loop() {
   compass.read();
   float heading = compass.heading();
   Serial.println(heading);
   delay(100);
}
