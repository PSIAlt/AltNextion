#include <Arduino.h>
#include "AltNextion.hpp"

#define PIN_LED PC13

#define NextionSerial Serial2
AltNextion::Display myNextion(NextionSerial);

void setup() {
    NextionSerial.begin(9600);
    pinMode(PIN_LED, OUTPUT);
    // Some other init
    //...
}

void getSliderPWM(const AltNextion::Message &msg) {
    // This function called when the Nextion display sends slider value change
    uint32_t val;
    // Yes we can run requests to the screen inside a message handler
    if( !myNextion.getComponentValue("led_slider", val) ) {
        return;
    }
    uint32_t m_val = map(val, 0, 100, 0, 255);
    analogWrite(PIN_LED, val);
}


void tick() {
    AltNextion::MessageWatcherTouchEvent sliderChange(myNextion, 1, 1); // Watch change event on page 1 object 1

    sliderChange.setCallback(getSliderPWM);

    myNextion.listen(100);
}
