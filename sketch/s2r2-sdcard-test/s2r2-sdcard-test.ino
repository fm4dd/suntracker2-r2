/* ------------------------------------------------- */
/* Suntracker2R2 MKR Zero SD Kartenleser Test Sketch */
/* ------------------------------------------------- */
#include <SPI.h>
#include <SD.h>

File root;

void setup() {
   Serial.begin(115200);
   while (!Serial);   // wait for serial port to connect.
   Serial.println("Serial OK");
   Serial.print("Initializing SD card...");
   if (!SD.begin(SDCARD_SS_PIN)) {
      Serial.println("initialization failed!");
      while(1);
   }
   Serial.println("initialization done.");
   root = SD.open("/");
   printDirectory(root, 0);
   Serial.println("done!");
}

void loop() {}

void printDirectory(File dir, int numTabs) {
   while (true) {
      File entry =  dir.openNextFile();
      if (! entry) break;     // no more files
      for (uint8_t i = 0; i < numTabs; i++) {
         Serial.print('\t');
      }
      Serial.print(entry.name());
      if (entry.isDirectory()) {
         Serial.println("/");
         printDirectory(entry, numTabs + 1);
      } else {              // show file size
         Serial.print("\t\t");
         Serial.println(entry.size(), DEC);
      }
      entry.close();
   }
}
