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
const int ROTARY_A = 16;
const int ROTARY_B = 17;

// Variables
float temperature;
float humidity;
float minTemperature = 20.0;
float maxTemperature = 25.0;
<<<<<<< HEAD
float minHumidity = 30.0;
float maxHumidity = 60.0;

=======
float minHumidity = 40.0;
float maxHumidity = 60.0;
>>>>>>> e7da3f7b1aacec4bc0dda1b52312bd1ee38c1308

// Config
const char *SSID = "VodaVice";
const char *PASS = "Dj0ab000";
String groupname = "BLM";

// Vectors
std::vector<float> temperatureVector;
std::vector<float> humidityVector;

// Server values
int lastTransmission = 0;
String serverTime;

// Blinking
long lastChange;
bool blinkRed = true;

DHT dht(DHT_PIN, DHT11);
Encoder minHumEncoder(ROTARY_A, ROTARY_B);
Encoder maxHumEncoder(ROTARY_A, ROTARY_B);
Encoder minTempEncoder(ROTARY_A, ROTARY_B);
Encoder maxTempEncoder(ROTARY_A, ROTARY_B);

enum SystemState
{
  TEMP_ISSUE,
  HUM_ISSUE,
  BOTH_ISSUE,
  SYSTEM_OK,
  MINHUMIDITY,
  MAXHUMIDITY,
  MINTEMPERATURE,
  MAXTEMPERATURE,
};

SystemState systemState = SYSTEM_OK;

// ***** FEATURE B *****

// Print change from/to green on serial port
void serialPrintChange(SystemState previous)
{
  if (previous != systemState)
  {
    // Changed to green
    if (systemState == SYSTEM_OK)
    {
      switch (previous)
      {
      case TEMP_ISSUE:
        Serial.println("** System State changed to Green : Caused by temperature **");
        break;
      case HUM_ISSUE:
        Serial.println("** System State changed to Green : Caused by humidity **");
        break;
      case BOTH_ISSUE:
        Serial.println("** System State changed to Green : Caused by temperature and humidity **");
        break;
      case SYSTEM_OK:
        break;
      }
    }

    // Changed from green
    if (previous == SYSTEM_OK)
    {
      switch (systemState)
      {
      case TEMP_ISSUE:
        Serial.println("** System State changed from Green to Red : Caused by temperature **");
        break;
      case HUM_ISSUE:
        Serial.println("** System State changed from Green to Blue : Caused by humidity **");
        break;
      case BOTH_ISSUE:
        Serial.println("** System State changed from Green to Red/Blue : Caused by temperature and humidity **");
        break;
      case SYSTEM_OK:
        break;
      }
    }
  }
}

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
  SystemState previousSystemState = systemState;
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

  serialPrintChange(previousSystemState);
}

void lightGreen()
{
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, HIGH);
  digitalWrite(PIN_BLUE, LOW);
}

void lightBlue()
{
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_BLUE, HIGH);
}

void lightRed()
{
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_BLUE, LOW);
}

void noColor()
{
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_BLUE, LOW);
}

void lightLED()
{
  switch (systemState)
  {
  case SYSTEM_OK:
    lightGreen();
    break;
  case TEMP_ISSUE:
    lightRed();
    break;
  case HUM_ISSUE:
    lightBlue();
    break;
  case BOTH_ISSUE:

    if (millis() - lastChange > 500)
    {
      lastChange = millis();
      blinkRed = !blinkRed;
    }
    if (blinkRed)
    {
      lightRed();
    }
    else
    {
      lightBlue();
    }
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

// ***** FEATURE D AND E *****
int lastState;
void checkButton()
{
  if (digitalRead(SWITCH) != systemState)
  {
    lastState = digitalRead(SWITCH);
    if (lastState == BOTH_ISSUE)
    {
      if (current == systemState::BOTH_ISSUE)
      {
        current = systemState::HUM_ISSUE;
      }
      else if (current == systemState::HUM_ISSUE)
      {
        current = systemState::TEMP_ISSUE;
      }
      else if (current == systemState::TEMP_ISSUE)
      {
        current = systemState::MINHUMIDITY;
      }
      else if (current == systemState::MINHUMIDITY)
      {
        minHumidity = minHumEncoder.getCount();
        minHumEncoder.setCount(minHumidity);
        Serial.println("Minimum Humidity set to: " + String(minHumidity) + "%");
        current = systemState::MAXHUMIDITY;
      }
      else if (current == systemState::MAXHUMIDITY)
      {
        maxHum = maxHumEncoder.getCount();
        maxHumEncoder.setCount(maxHumidity);
        Serial.println("Maximum Humidity set to: " + String(maxHumidity) + "%");
        current = State::MINTEMPERATURE;
      }
      else if (current == State::MINTEMPERATURE)
      {
        minTemperature = minTempEncoder.getCount();
        minTempEncoder.setCount(minTemperature);
        Serial.println("Minimum Temperature set to: " + String(minTemperature) + "\370C");
        current = systemState::MAXTEMPERATURE;
      }
      else if (current == systemState::MAXTEMPERATURE)
      {
        maxTemperature = maxTempEncoder.getCount();
        maxTempEncoder.setCount(maxTemperature);
        Serial.println("Maximum Temperature set to: " + String(maxTemperature) + "\370C");
        current = systemState::TEMP_ISSUE;
      }
    }
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
  }
}

// ***** FEATURE A *****
void testLED()
{
  Serial.println("LED test.");

  // color RED
  Serial.println("Lighting LED red.");
  lightRed();

  delay(2000); // keep the color 2 seconds

  // color GREEN
  Serial.println("Lighting LED green.");
  lightGreen();

  delay(2000); // keep the color 2 seconds

  // color BLUE
  Serial.println("Lighting LED blue.");
  lightBlue();

  delay(2000); // keep the color 2 seconds

  // Reset color
  noColor();

  Serial.println("LED test finished.");
}

void testDHT()
{
  Serial.println("DHT test starting...");

  // Read humidity
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and if any do the system will shutdown
  if (isnan(humidity) || isnan(temperature) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor! Shutting system down...");
    void shutdown();
  }

  Serial.println("DHT test successful.");
}

bool sdCardTest()
{
  Serial.println("Beginning SD test...");

  // Write test file
  String testFileName = "/test.txt";
  String testFileString = "SD Card Test";

  // if (SD.)
  // {
  //   Serial.println("Error writing the file: SD Card Test Failed");
  //   return false;
  // }

  File file = SD.open(testFileName, FILE_WRITE);

  if (!file)
  {
    Serial.println("SD Test Failed. Feature G Disabled.");
    SD.end();
    return false;
  }

  file.println(testFileString);
  Serial.println("Writing to file...");
  file.close();

  // Read Data -- if no value then fail

  file = SD.open(testFileName);
  String readLine = file.readStringUntil('\r');
  Serial.println("Reading file...");
  readLine.trim();
  if (readLine.equals(testFileString))
  {
    // Serial.println(readLine);
    Serial.println("SD Test Passed.");
    file.close();
    SD.remove(testFileName);
    return true;
  }
  else
  {
    Serial.println("SD Test Failed. Feature G Disabled.");
    file.close();
    SD.remove(testFileName);
    return false;
  }
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
  SD.begin(CS_PIN);
  dht.begin();
  screen = new tftScreen(&tft);
  lastChange = millis();

  // Feature A
  testLED();
  connectToWiFi();
  testDHT();
  bool isSdCardConnected = sdCardTest();
  Serial.println("System passed all neccessary checks.");
  // Determine system status before startup
  updateSystemStatus();

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

  // If system passed sd card test
  if (isSdCardConnected)
  {
    // Feature G
    xTaskCreate(
        writeValues,
        "Saving Values",
        6000,
        NULL,
        4,
        NULL);
  }
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------

void loop()
{
  // Feature B

  float newTemperature = dht.readTemperature();
  float newHumidity = dht.readHumidity();
  // Check if the readings are valid
  if (!isnan(newTemperature) || !isnan(newHumidity))
  {
    temperature = newTemperature;
    humidity = newHumidity;
  }
  updateSystemStatus();

  lightLED();

  // Feature H
  screen->updateTft(temperature, humidity);
}
