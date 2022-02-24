#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <TFT_eSPI.h>                

const int CS_PIN = 5;
String fileName = "/arduino.txt";

#include <TFT_eSPI.h>                 // Include the graphics library (this includes the sprite functions)

TFT_eSPI    tft = TFT_eSPI();         // Create object "tft"

// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   pinMode(2, OUTPUT);
// }

void writeFile() {
  SD.begin(CS_PIN);

  File file = SD.open(fileName, FILE_WRITE);
  file.println("Hello world");
  file.close();

  SD.end();
}

void readFile() {
  SD.begin(CS_PIN);

  File file = SD.open(fileName);
  while (file.available()) {
    String readLine = file.readStringUntil('\r');
    readLine.trim();
    if (readLine.length() > 0) {
      Serial.println(readLine);
    }
  }
  file.close();

  SD.end();
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop() {
  // Wrap text at right and bottom of screen
  tft.setTextWrap(true, true);
  tft.setTextPadding(2.5);

  // Font and background colour, background colour is used for anti-alias blending
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  // Set "cursor" at top left corner of display (0,0)
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(5, 5);

  // Print text on the screen
  tft.print("Temp:");

  // Set the font colour to be magenta
  tft.setTextColor(TFT_MAGENTA);
  tft.println(23);
}
