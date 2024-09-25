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

    // Send custom commands using Nextion language
    myNextion.sendCommand("page home");
}

void pageChangeEvent(const AltNextion::Message &msg) {
    // ... do something with msg.payload_u8,msg.payload_u16,msg.payload_u32, decode raw or custom data etc ...

}


void tick() {
    //AltNextion::MessageWatcherTouchEvent anyChange(myNextion); // Watch ANY event
    AltNextion::MessageWatcherTouchEvent pageChange(myNextion, 1); // Watch ANY event on page 1

    pageChange.setCallback(pageChangeEvent);

    myNextion.listen(100);
}
