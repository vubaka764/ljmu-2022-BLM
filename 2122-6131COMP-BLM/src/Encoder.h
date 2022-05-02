#include <ESP32Encoder.h>

class Encoder
{
private:
    int encodercnt = 0;
    ESP32Encoder encoder;
public:
    Encoder(int pinA, int pinB);
    int rangeCheckMin(int val);
    int rangeCheckMax(int val);
    int getCount();
    void setCount(int val);
    void reset();
};