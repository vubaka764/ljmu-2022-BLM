/*
 * screen.h
 *
 * Author: Sebastian
 */

#include "Arduino.h"
#include <TFT_eSPI.h>   

#ifndef SCREEN_H_
#define SCREEN_H_

class tftScreen {
private: 
    TFT_eSPI* tft = nullptr;
	float temperature;
    float humidity;
    void printLine(String heading, String value);
public:
	tftScreen(TFT_eSPI* tft);
    void updateTft(float Temperature, float Humidity);
};

#endif /* SCREEN_H_ */
