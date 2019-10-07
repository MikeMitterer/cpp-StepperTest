//
// Created by Mike Mitterer on 01.11.17.
//

#ifndef CATDOOR_ONELED_H
#define CATDOOR_ONELED_H


#include <avr/io.h>

class OneLED {
public:
    const uint8_t id;

public:
    OneLED() = delete;
    explicit OneLED(uint8_t id);

    void init(boolean turnOn = false);

    void on();
    void off();
    void onoff(const uint8_t delay = 10);
};


#endif //CATDOOR_ONELED_H
