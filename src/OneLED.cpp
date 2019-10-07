//
// Created by Mike Mitterer on 01.11.17.
//

#include <Arduino.h>
#include "OneLED.h"

OneLED::OneLED(const uint8_t id) : id(id) {}

void OneLED::init(const boolean turnOn) {
    pinMode(id, OUTPUT);
    
    digitalWrite(id,turnOn ? HIGH : LOW);
}

void OneLED::on() {
    digitalWrite(id,HIGH);
}

void OneLED::off() {
    digitalWrite(id,LOW);
}

void OneLED::onoff(const uint8_t delay) {
    on();
    ::delay(delay);
    off();
}
