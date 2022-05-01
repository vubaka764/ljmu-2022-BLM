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
  SCK = 18         **BLUE****
  SO = 19          **ORANGE**
  SI = 23          **GREEN**
  TCS = 15         **ORANGE**
  RST = 4          **PURPLE**
  D/C = 2          **YELLOW**
--- SD CARD ---
  CCS = 5          **BLACK**
--- DHT ---
  DHT = 16         **WHITE**
--- LED ---
  RED = 0
  GREEN = 22
  BLUE = 21

**/

// Classes
#include <screen.h>

TFT_eSPI tft = TFT_eSPI();
tftScreen *screen = nullptr;

// PINS
const int CS_PIN = 5;
const int DHT_PIN = 16;
const int PIN_RED = 0;
const int PIN_GREEN = 22;
const int PIN_BLUE = 21;

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

// Vectors
std::vector<float> temperatureVector;
std::vector<float> humidityVector;

// Server values
int lastTransmission = 0;
String serverTime;

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

// ***** FEATURE B *****
void lightLED()
{
  switch (systemState)
  {
  case SYSTEM_OK:
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_GREEN, HIGH);
    digitalWrite(PIN_BLUE, LOW);
    break;
  case TEMP_ISSUE:
    digitalWrite(PIN_RED, HIGH);
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_BLUE, LOW);
    break;
  case HUM_ISSUE:
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_BLUE, HIGH);
    break;
  case BOTH_ISSUE:
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_BLUE, LOW);
    break;
  }
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

void sendValues(void *parameters)
{
  for (;;)
  {
    vTaskDelay(30000 / portTICK_PERIOD_MS);
    int vectorSize = temperatureVector.size();
    if (vectorSize > 0)
    {

      String url = "http://blm.atwebpages.com/blm.php?group=";
      url.concat(groupname);
      url.concat("&t=");
      String tempReadings;
      String humReadings;

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
          serverTime = hClient.header("Date");
          lastTransmission = vectorSize;
          // Serial.println("Date = " + hClient.header("Date"));
        }
        else
        {
          Serial.println("Error transmitting data...");
          Serial.println(HTTPClient::errorToString(retCode));
        }
      }
    }
  }
}

// ***** FEATURE G *****
void writeValues(void *parameters)
{
  for (;;)
  {
    vTaskDelay(120000 / portTICK_PERIOD_MS);

    SD.begin(CS_PIN);

    serverTime.replace(",", "");
    serverTime.replace(" ", "_");
    serverTime.replace(":", "_");

    String fileName = "/" + serverTime + ".txt";

    // Serial.println(fileName);
    File file = SD.open(fileName, FILE_WRITE);

    int vectorSize = temperatureVector.size();
    for (size_t i = lastTransmission; i < vectorSize; ++i)
    {
      file.print("Temperature: ");
      file.print(String(temperatureVector[i], 0));
      file.print(", Humidity: ");
      file.println(String(humidityVector[i], 0));
    }

    file.close();
    Serial.println("Wrote to file.");

    temperatureVector.clear();
    humidityVector.clear();
    lastTransmission = 0;

    SD.end();
  }
}

// ***** FEATURE A *****
void testLED(void)
{
  Serial.println("LED test.");
  // color RED

  Serial.println("Lighting LED red.");
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_BLUE, LOW);

  delay(2000); // keep the color 2 seconds

  // color GREEN
  Serial.println("Lighting LED green.");
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, HIGH);
  digitalWrite(PIN_BLUE, LOW);

  delay(2000); // keep the color 2 seconds

  // color BLUE
  Serial.println("Lighting LED blue.");
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_BLUE, HIGH);

  delay(2000); // keep the color 2 seconds

  Serial.println("LED test finished.");
}

void testDHT(void)
{
  Serial.println("DHT test starting...");

  // Read humidity
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and if it any do the system shutsdown
  if (isnan(humidity) || isnan(temperature) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor! Shutting system down...");
    void shutdown();
  }
  Serial.println("DHT test successful.");
}

void sdCardTest(void)
{
  SD.begin(CS_PIN);

  Serial.println("SD test");

  // Write test file
  String testFileName = "test.txt";

  File file = SD.open(testFileName, FILE_WRITE);
  file.println("SD Card Test");
  file.close();
  Serial.println("Wrote to file.");

  // TODO: ** Read Data ** if no value then fail
  
  // File file = SD.open(testFileName);
  // while (file.available())
  // {
  //   String readLine = file.readStringUntil('\r');
  //   readLine.trim();
  //   if (readLine.length() > 0)
  //   {
  //     Serial.println(readLine);
  //   }
  // }
  // file.close();

  // Serial.println("System Fail");
  // void shutdown();

  // TODO: ** delete file after test is done **

  SD.end();
}

void connectToWiFi(void)
{
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);

  // TODO: Shutdown the system if connection is not made in 20 seconds
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.print("Connected as : ");
  Serial.println(WiFi.localIP());
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------

void setup(void)
{
  // Set up
  Serial.begin(115200);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  dht.begin();
  screen = new tftScreen(&tft);

  // Feature A
  testLED();
  connectToWiFi();
  testDHT();
  Serial.println("System passed all checks.");

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

  // Feature G
  xTaskCreate(
      writeValues,
      "Saving Values",
      6000,
      NULL,
      4,
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
  lightLED();
  //Feature H
  screen->updateTft(temperature, humidity);

}
