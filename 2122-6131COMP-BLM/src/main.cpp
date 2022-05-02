// Libraries
#include <Arduino.h>
#include <Encoder.h>
#include <ESP32Encoder.h>
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
const int ROTARY_A = 14;
const int ROTARY_B = 12;
const int SWITCH_PIN = 27;
const int TEMPHUM_PIM = 13;

// Variables
float temperature;
float humidity;
float minTemperature = 20.0;
float maxTemperature = 25.0;
float minHumidity = 40.0;
float maxHumidity = 60.0;
bool isAdjustingMax;
bool isAdjustingTemperature = true;
long lastButtonPress;

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

// Blinking
long lastChange;
bool blinkRed = true;

DHT dht(DHT_PIN, DHT11);
ESP32Encoder encoder;

enum SystemState
{
  TEMP_ISSUE,
  HUM_ISSUE,
  BOTH_ISSUE,
  SYSTEM_OK,
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
void checkSwitch()
{
  if (digitalRead(SWITCH_PIN) != isAdjustingMax)
  {

    if (digitalRead(SWITCH_PIN))
    {
      isAdjustingMax = true;
      if (isAdjustingTemperature)
      {
        encoder.setCount(maxTemperature);
      }
      else
      {
        encoder.setCount(maxHumidity);
      }

      Serial.println("[Switch] Switched to adjusting max value");
    }
    else
    {
      isAdjustingMax = false;
      if (isAdjustingTemperature)
      {
        encoder.setCount(minTemperature);
      }
      else
      {
        encoder.setCount(minHumidity);
      }
      Serial.println("[Switch] Switched to adjusting min value");
    }
  }
}

void checkTempHumButton()
{
  if (!digitalRead(TEMPHUM_PIM))
  {

    if (millis() - lastButtonPress > 1000)
    {
      lastButtonPress = millis();
      isAdjustingTemperature = !isAdjustingTemperature;
      if (isAdjustingTemperature)
      {
        Serial.println("[Button Press] Switched to adjusting temperature");
        if (isAdjustingMax) {
          encoder.setCount(maxTemperature);
        } else {
          encoder.setCount(minTemperature);
        }
      }
      else
      {
        Serial.println("[Button Press] Switched to adjusting humidity");
                if (isAdjustingMax) {
          encoder.setCount(maxHumidity);
        } else {
          encoder.setCount(minHumidity);
        }
      }
    }
  }
}

long lastButtonSerialLog;

void updateTemperature(float encoderCount)
{
  if (isAdjustingMax)
  {
    if (maxTemperature != encoderCount)
    {

      if (encoderCount >= 30)
      {
        maxTemperature = 30;
        encoder.setCount(30);
      }
      else if (encoderCount <= minTemperature + 1)
      {
        maxTemperature = minTemperature + 1;
        encoder.setCount(maxTemperature);
      }

      maxTemperature = encoder.getCount();
      if (millis() - lastButtonSerialLog > 250)
      {
        Serial.println("[Encoder] New max temperature: " + String(maxTemperature));
        lastButtonSerialLog = millis();
      }
    }
  }
  else
  {
    if (minTemperature != encoderCount)
    {

      if (encoderCount >= maxTemperature - 1)
      {
        minTemperature = maxTemperature - 1;
        encoder.setCount(minTemperature);
      }
      else if (encoderCount <= 5)
      {
        minTemperature = 5;
        encoder.setCount(minTemperature);
      }

      minTemperature = encoder.getCount();
      if (millis() - lastButtonSerialLog > 250)
      {
        Serial.println("[Encoder] New min temperature: " + String(minTemperature));
        lastButtonSerialLog = millis();
      }
    }
  }
}

void updateHumidity(float encoderCount)
{
  if (isAdjustingMax)
  {
    if (maxHumidity != encoderCount)
    {

      if (encoderCount >= 100)
      {
        maxHumidity = 100;
        encoder.setCount(100);
      }
      else if (encoderCount <= minHumidity + 1)
      {
        maxHumidity = minHumidity + 1;
        encoder.setCount(maxHumidity);
      }

      maxHumidity = encoder.getCount();
      if (millis() - lastButtonSerialLog > 250)
      {
        Serial.println("[Encoder] New max humidity: " + String(maxHumidity));
        lastButtonSerialLog = millis();
      }
    }
  }
  else
  {
    if (minHumidity != encoderCount)
    {

      if (encoderCount >= maxHumidity - 1)
      {
        minHumidity = maxHumidity - 1;
        encoder.setCount(minHumidity);
      }
      else if (encoderCount <= 0)
      {
        minHumidity = 0;
        encoder.setCount(minHumidity);
      }

      minHumidity = encoder.getCount();
      if (millis() - lastButtonSerialLog > 250)
      {
        Serial.println("[Encoder] New min humidity: " + String(minHumidity));
        lastButtonSerialLog = millis();
      }
    }
  }
}

void checkButton()
{
  float encoderCount = encoder.getCount();
  if (isAdjustingTemperature) {
    updateTemperature(encoderCount);
  } else {
    updateHumidity(encoderCount);
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
  pinMode(SWITCH_PIN, INPUT);
  pinMode(TEMPHUM_PIM, INPUT);
  SD.begin(CS_PIN);
  dht.begin();
  screen = new tftScreen(&tft);

  // Encoder Set up
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(ROTARY_A, ROTARY_B);
  isAdjustingMax = digitalRead(SWITCH_PIN);
  if (isAdjustingMax)
  {
    encoder.setCount(maxTemperature);
  }
  else
  {
    encoder.setCount(minTemperature);
  }
  lastButtonSerialLog = millis();

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

  // Feature D and E
  checkSwitch();
  checkButton();
  checkTempHumButton();

  // Feature H
  screen->updateTft(temperature, humidity);
}
