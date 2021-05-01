#include <Arduino.h>

#include "nymealight.h"

// TEST
NymeaLight *light = nullptr;

void setup() {
    // Get rid of unused warning from lib
    (void)&_modes;

    light = new NymeaLight(Serial, new WS2812FX(10, 13, NEO_GRB + NEO_KHZ800));
    light->init();
}

void loop() {
    light->process();
}


