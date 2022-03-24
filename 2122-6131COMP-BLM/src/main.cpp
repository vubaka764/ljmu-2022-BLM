// Libraries
#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <TFT_eSPI.h>      
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>  
#include <DHT.h>  

/** 
 
------ PINS : ------
 
--- SCREEN ---
  SCK = 18         **BLUE**
  SO = 19          **ORANGE**
  SI = 23          **GREEN**
  TCS = 15         **ORANGE**
  RST = 4          **PURPLE**
  D/C = 2          **YELLOW**
--- SD CARD ---
  CCS = 5          **BLACK**
--- DHT ---
  DHT = 16         **WHITE**

**/

// Classes
#include <screen.h>

TFT_eSPI tft = TFT_eSPI();  
tftScreen* screen = nullptr;
const int CS_PIN = 5;
const int DHT_PIN = 16;
//String fileName = "/arduino.txt";

// Variables
float temperature;
float humidity;
float minTemperature = 21.0;
float maxTemperature = 25.0;
float minHumidity = 20.0;
float maxHumidity = 50.0;

DHT dht(DHT_PIN, DHT11);

enum SystemState {
  TEMP_ISSUE,
  HUM_ISSUE,
  BOTH_ISSUE,
  SYSTEM_OK,
};

SystemState systemState = SYSTEM_OK;

// Update System Status
void updateSystemStatus() {
  boolean tempIssue = false;
  boolean humIssue = false;

  // Check if there are issues
  if (temperature < minTemperature || temperature > maxTemperature) {
    tempIssue = true;
  } 
  if (humidity < minHumidity || humidity > maxHumidity) {
    humIssue = true;
  }

  // Update systemState based on the issues
  if (!tempIssue && !humIssue) {
    systemState = SYSTEM_OK;
  } else if (tempIssue && !humIssue) {
    systemState = TEMP_ISSUE;
  } else if (!tempIssue && humIssue) {
    systemState = HUM_ISSUE;
  } else if (tempIssue && humIssue) {
    systemState = BOTH_ISSUE;
  }
}

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

// ***** FEATURE C *****
void logSerial(void * parameters) {
  for(;;) {
    switch(systemState) {
      case SYSTEM_OK:
        Serial.print("[GREEN - SYSTEM_OK] Humidity and temperature within set ranges. ");
        break;
      case TEMP_ISSUE:
        Serial.print("[RED - TEMP_ISSUE] Temperature outside set range. ");
        break;
      case HUM_ISSUE:
        Serial.print("[BLUE - HUM_ISSUE] Humidity outside set range. ");
        break;
      case BOTH_ISSUE:
        Serial.print("[RED/BLUE - BOTH_ISSUE] Both humidity and temperature outside range. ");
        break;
    }
    String tempRange = " (" + String(minTemperature,0) + "-" + String(maxTemperature,0) + ")";
    String humRange = " (" + String(minHumidity,0) + "-" + String(maxHumidity,0) + ")";
    Serial.println("Temperature: " + String(temperature,0) + tempRange + " Humidity: " + String(humidity,0) + humRange);
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------

void setup(void) {
  Serial.begin(115200);

  dht.begin();

  screen = new tftScreen(&tft);

  // Feature C
  xTaskCreate(
    logSerial,
    "Serial Logging",
    1000,
    NULL,
    1,
    NULL
  );
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------

void loop() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  updateSystemStatus();

  // Feature H
  // screen->updateTft(dht.readTemperature(), dht.readHumidity());

}
