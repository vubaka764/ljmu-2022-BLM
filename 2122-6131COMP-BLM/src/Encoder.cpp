#include "Encoder.h"


Encoder::Encoder(int pinA, int pinB)
{
  ESP32Encoder::useInternalWeakPullResistors = UP;
  //encoder.attachHalfQuad(pinA, pinB); //increment by 2
  encoder.attachSingleEdge(pinA, pinB); //encoder to increase by 1
}

// range checks the encode
int Encoder::rangeCheckMin(int val)
{
  if (encodercnt != encoder.getCount())
  {
    encodercnt = encoder.getCount();

    if (encodercnt >= val)
    {
      encodercnt = val -1;
      encoder.setCount(encodercnt);

    }
    else if (encodercnt <= 0)
    {
      encodercnt = 0;
      encoder.setCount(encodercnt);
    }
  }
  return encodercnt;
  encodercnt=0;
}

int Encoder::rangeCheckMax(int val)
{
  if (encodercnt != encoder.getCount())
  {
    encodercnt = encoder.getCount();

    if (encodercnt >= 100)
    {
      encodercnt = 100;
      encoder.setCount(encodercnt);
    }
    else if (encodercnt <= val)
    {
      encodercnt = val +1;
      encoder.setCount(encodercnt);
    }
  }
  return encodercnt;
    encodercnt=0;
}

int Encoder::getCount()
{
  return encoder.getCount();
  
}

void Encoder::setCount(int val)
{
  encoder.setCount(val);
}
