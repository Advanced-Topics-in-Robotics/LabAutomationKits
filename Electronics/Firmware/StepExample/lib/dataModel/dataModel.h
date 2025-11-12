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


extern Pump pumpA;
extern Pump pumpB;
extern Pump pumpC;


#endif