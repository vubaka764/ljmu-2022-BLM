# ljmu-2022-BLM

### Instructions
Use the switch on rotary encoder to switch between adjusting min and max temperature.
Rotate to increase and decrease the value. Button press toggles between adjusting temperature and humidity.

### Neccessary libraries:

#### TFT eSPI
https://github.com/Bodmer/TFT_eSPI?utm_source=platformio&utm_medium=piohome

#### Adafruit Unified Sensor
https://github.com/adafruit/Adafruit_Sensor?utm_source=platformio&utm_medium=piohome

#### ESP32Encoder
https://github.com/madhephaestus/ESP32Encoder?utm_source=platformio&utm_medium=piohome

#### DHT Sensor Library
https://github.com/adafruit/DHT-sensor-library?utm_source=platformio&utm_medium=piohome

### Pins:

#### --- SCREEN ---
  SCK = 18
  SO = 19
  SI = 23
  TCS = 15
  RST = 4
  D/C = 2         
#### --- SD CARD ---
  CCS = 5
#### --- DHT ---
  DHT = 16
#### --- LED ---
  RED = 0
  GREEN = 22
  BLUE = 21
#### --- ROTARY ENCODER ---
  ROTARY_A = 14
  ROTARY_B = 12
  SWITCH_PIN = 27
  BTN_PIN = 13
