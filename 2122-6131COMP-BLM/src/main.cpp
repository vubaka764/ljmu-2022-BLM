#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <TFT_eSPI.h>      
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>  
#include <DHT.h>  

//PINS : 
// SCK = 18  **BLUE**
// SO = 19 **ORANGE**
// SI = 23 **GREEN**
// TCS = 15 **ORANGE**
// RST = 4 **PURPLE**
// D/C = 2 **YELLOW**
// CCS = 5 **BLACK**
// DHT = 16 **WHITE**

const int CS_PIN = 5;
const int DHT_PIN = 16;
String fileName = "/arduino.txt";

TFT_eSPI    tft = TFT_eSPI();    
DHT dht(DHT_PIN, DHT11);

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

void printLine(String heading, String value) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print(heading + ":");
  tft.setTextColor(TFT_MAGENTA);
  tft.println(value);
}

void printOnScreen(String temperature, String humidity) {
  tft.fillScreen(TFT_BLACK);
  // Wrap text at right and bottom of screen and set text size
  tft.setTextWrap(true, true);
  tft.setTextSize(1.8);
  tft.setCursor(0,10);

  // Print Temperature and humidity
  printLine("Temperature", temperature);
  printLine("Humidity", humidity);
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);

  dht.begin();
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop() {
  printOnScreen(String(dht.readTemperature(), 0), String(dht.readHumidity(), 0));
  delay(1000);
}
