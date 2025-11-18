#ifndef DATAMODEL_H_
#define DATAMODEL_H_


#include <Arduino.h>



class Pump{
    public:
        bool state = false;
        uint16_t speed = 0;
        bool dir = true;
        unsigned long stepStartTime;
        unsigned long time;
        bool done = false;
};

class Color{
    public:
        uint16_t red = 0;
        uint16_t green = 0;
        uint16_t blue = 0;
        uint16_t nir = 0;
};


extern Pump pumpA;
extern Pump pumpB;
extern Pump pumpC;

extern Color color;


#endif