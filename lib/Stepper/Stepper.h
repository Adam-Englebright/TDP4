// Library header for implementing a class for control of the stepper motor driver.

#ifndef _STEPPER_H
#define _STEPPER_H

#include "pico/stdlib.h"

class Stepper
{
    uint enable;
    uint sleep;
    uint reset;
    uint step;
    uint dir;
    uint ms1;
    uint ms2;
    uint ms3;
    uint slice_num;
public:
    // Constructor will take stepper frequency, and gpio ID numbers that are used to interface with the driver.
    Stepper(uint, uint, uint, uint, uint, uint, uint, uint, uint);
    // Method to start moving actuator forwards.
    void forward(void);
    // Method to start moving actuator backwards.
    void backward(void);
    // Method to stop actuator.
    void stop(void);
};

#endif