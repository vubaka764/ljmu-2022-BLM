/*
 * screen.cpp
 *
 * Author: Sebastian
 */

#include <screen.h>
#include <TFT_eSPI.h>   

#include <esp32-hal.h>
#include <esp32-hal-gpio.h>

tftScreen::tftScreen(TFT_eSPI* xtft) {
    
  this->tft = xtft;

  this->tft->init();
  this->tft->setRotation(0);
  this->tft->fillScreen(TFT_BLACK);
}

void tftScreen::printLine(String heading, String value) {
  this->tft->setTextColor(TFT_GREEN, TFT_BLACK);
  this->tft->print(heading + ":");
  this->tft->setTextColor(TFT_MAGENTA, TFT_BLACK);
  this->tft->println(value);
}

void tftScreen::updateTft(float temperature, float humidity) {

  // Wrap text at right and bottom of screen and set text size
  tft->setTextWrap(true, true);
  tft->setTextSize(1.8);
  tft->setCursor(0,10);

  // Print Temperature and humidity
  printLine("Temperature", String(temperature, 0));
  printLine("Humidity", String(humidity, 0));
}