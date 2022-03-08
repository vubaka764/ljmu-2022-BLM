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

#include <screen.h>

TFT_eSPI tft = TFT_eSPI();  
tftScreen* screen = nullptr;
const int CS_PIN = 5;
const int DHT_PIN = 16;
String fileName = "/arduino.txt";

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

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  Serial.begin(115200);

  dht.begin();

  screen = new tftScreen(&tft);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop() {
  screen->updateTft(dht.readTemperature(), dht.readHumidity());
  delay(1000);
}
