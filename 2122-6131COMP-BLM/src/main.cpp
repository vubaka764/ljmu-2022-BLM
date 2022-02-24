#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <TFT_eSPI.h>                

const int CS_PIN = 5;
String fileName = "/arduino.txt";

#include <TFT_eSPI.h>                 // Include the graphics library (this includes the sprite functions)

TFT_eSPI    tft = TFT_eSPI();         // Create object "tft"

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

void printOnScreen(String temperature, String humidity) {
  // Wrap text at right and bottom of screen and set text size
  tft.setTextWrap(true, true);
  tft.setTextSize(2);

  // Print Temperature and humidity
  printLine("Temp", temperature);
  printLine("Humidity", humidity);
}

void printLine(String heading, String value) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(5, 5);
  tft.print(heading + ":");
  tft.setTextColor(TFT_MAGENTA);
  tft.println(value);
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop() {
  printOnScreen("22", "23");
}
