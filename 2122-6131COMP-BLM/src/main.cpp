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
#include <vector>

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
tftScreen *screen = nullptr;
const int CS_PIN = 5;
const int DHT_PIN = 16;
String fileName = "/esp32.txt";

// Variables
float temperature;
float humidity;
float minTemperature = 21.0;
float maxTemperature = 25.0;
float minHumidity = 20.0;
float maxHumidity = 50.0;

// Config
const char *SSID = "";
const char *PASS = "";
String groupname = "BLM";

std::vector<float> temperatureVector;
std::vector<float> humidityVector;

int lastTransmission = 0;

DHT dht(DHT_PIN, DHT11);

enum SystemState
{
  TEMP_ISSUE,
  HUM_ISSUE,
  BOTH_ISSUE,
  SYSTEM_OK,
};

SystemState systemState = SYSTEM_OK;

// Update System Status
void updateSystemStatus()
{
  boolean tempIssue = false;
  boolean humIssue = false;

  // Check if there are issues
  if (temperature < minTemperature || temperature > maxTemperature)
  {
    tempIssue = true;
  }
  if (humidity < minHumidity || humidity > maxHumidity)
  {
    humIssue = true;
  }

  // Update systemState based on the issues
  if (!tempIssue && !humIssue)
  {
    systemState = SYSTEM_OK;
  }
  else if (tempIssue && !humIssue)
  {
    systemState = TEMP_ISSUE;
  }
  else if (!tempIssue && humIssue)
  {
    systemState = HUM_ISSUE;
  }
  else if (tempIssue && humIssue)
  {
    systemState = BOTH_ISSUE;
  }
}

void writeFile()
{
  SD.begin(CS_PIN);

  File file = SD.open(fileName, FILE_WRITE);
  // file.println("");
  file.close();

  SD.end();
}

void readFile()
{
  SD.begin(CS_PIN);

  File file = SD.open(fileName);
  while (file.available())
  {
    String readLine = file.readStringUntil('\r');
    readLine.trim();
    if (readLine.length() > 0)
    {
      Serial.println(readLine);
    }
  }
  file.close();

  SD.end();
}

// ***** FEATURE C *****
void logSerial(void *parameters)
{
  for (;;)
  {
    switch (systemState)
    {
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
    String tempRange = " (" + String(minTemperature, 0) + "-" + String(maxTemperature, 0) + ")";
    String humRange = " (" + String(minHumidity, 0) + "-" + String(maxHumidity, 0) + ")";
    Serial.println("Temperature: " + String(temperature, 0) + tempRange + " Humidity: " + String(humidity, 0) + humRange);

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// ***** FEATURE F *****
void logReadings(void *parameters)
{
  for (;;)
  {

    temperatureVector.push_back(temperature);
    humidityVector.push_back(humidity);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void sendValues(void *paramters)
{
  for (;;)
  {
    String url = "http://blm.atwebpages.com/blm.php?group=";
    url.concat(groupname);
    url.concat("&t=");
    String tempReadings;
    String humReadings;
    int vectorSize = temperatureVector.size();
    for (size_t i = lastTransmission; i < vectorSize; ++i)
    {
      tempReadings.concat(String(temperatureVector[i], 0) + ',');
      humReadings.concat(String(humidityVector[i], 0) + ',');
    }

    url.concat(tempReadings);
    url.concat("&h=");
    url.concat(humReadings);
    // Serial.println(url);
    HTTPClient hClient;

    hClient.begin(url);
    const char *headers[] = {"Date"};
    hClient.collectHeaders(headers, 1);
    hClient.setTimeout(3500);
    int retCode = hClient.GET();
    if (retCode > 0)
    {
      // a real HTTP code
      // Serial.print("HTTP ");
      // Serial.println(retCode);
      if (retCode == HTTP_CODE_OK)
      {
        Serial.println("Readings transmitted | " + hClient.header("Date"));
        lastTransmission = vectorSize;
        // Serial.println("Date = " + hClient.header("Date"));
      }
      else
      {
        Serial.println("Error transmitting data...");
        Serial.println(HTTPClient::errorToString(retCode));
      }
    }

    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

// for (auto log : *dynVector) {
//
// }
// dynVector->clear();

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);

  delay(5000);

  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.print("Connected as : ");
  Serial.println(WiFi.localIP());

  dht.begin();

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  screen = new tftScreen(&tft);

  // Feature C
  xTaskCreate(
      logSerial,
      "Serial Logging",
      1000,
      NULL,
      1,
      NULL);

  // Feature F
  xTaskCreate(
      logReadings,
      "Readings Logging",
      1000,
      NULL,
      2,
      NULL);

  xTaskCreate(
      sendValues,
      "Sending Values",
      3000,
      NULL,
      3,
      NULL);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------

void loop()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  updateSystemStatus();

  // Feature H
  // screen->updateTft(temperature, humidity);
}
