#include <Arduino.h>

#include "AltNextion.hpp"

namespace AltNextion {
const uint8_t endbyte = 0xff;

bool Display::listen(long timeout_ms) {
    unsigned long last_ms = millis();
    const int delay_ms = 1;
    delay(5);
    while( serial.available()>0 && timeout_ms>delay_ms ){
        unsigned long m = millis();
        timeout_ms -= (m-last_ms);
        last_ms=m;
        serial.setTimeout( timeout_ms > delay_ms ? timeout_ms : delay_ms );
        if( rdbuf_len >= sizeof(rdbuf) ) {
            // Posible overflow..sadly throw out everything...
            rdbuf_len=1;
            rdbuf[0]=0;// fake command code
        }
        size_t rd = serial.readBytes(&rdbuf[rdbuf_len], 3-endcount);
        for(int i=rdbuf_len; i<(rdbuf_len+rd); i++) {
            if( rdbuf[i] == endbyte )
                endcount++;
        }
        rdbuf_len += rd;
        if( endcount > 3 )
            abort(); //omg something wrong happend
        if( endcount == 3 ) {
            int ofs = rdbuf_len -endcount -1; //-1 cus command code is outside this buf
            // assert( ofs < sizeof(msg.payload_u8));
            msg.payload_u8[ofs] = 0;
            endcount = 0;
            rdbuf_len = 0;
            dispatchEvent();
            return true;
        }
    }

    return false;
}

bool Display::sendCommand(const char *cmd, int len) {
    if( len < 0 )
        len = strlen(cmd);
    serial.write(cmd, len);
    serial.write(0xFF);
    serial.write(0xFF);
    serial.write(0xFF);
    return true; //serial.write has "no real error management" lol..
}

bool Display::waitAnswerWithWatcher(MessageWatcher &w) {
    for(int i=0; i<10 && !w.hadMessage(); i++) {
        delay(10);
        listen(90);
    }
    return w.hadMessage();
}

#define GEN_GET_Q(name, l) char buf[sizeof("get ") + l + sizeof(".val") + 10/*zero etc*/] = "get "; \
    strcat(buf, name); \
    strcat(buf, ".val");

bool Display::getComponentValue(const char *name, uint32_t &val) {
    MessageWatcherNumber w(*this);
    int l = strlen(name);
    GEN_GET_Q(name, l);
    sendCommand(buf);
    if( waitAnswerWithWatcher(w) ) {
        val = w.getValue();
        return true;
    }
    return false;
}

bool Display::getComponentValue(const char *name, String &val) {
    MessageWatcherString w(*this);
    int l = strlen(name);
    GEN_GET_Q(name, l);
    sendCommand(buf);
    if( waitAnswerWithWatcher(w) ) {
        val = w.getValue();
        return true;
    }
    return false;

}

void Display::dispatchEvent() {
    Message msg_copy; // In case when dispatch handler issuing commands
    memcpy(&msg_copy, &msg, sizeof(msg));
    MessageWatcher *w = watchers_head;
    while( w ) {
        w->checkMessage(msg_copy);
        w = w->watcher_next;
    }
}

void Display::addWatcher(MessageWatcher *w) {
    if( watchers_head == NULL ) {
        watchers_head = w;
    } else {
        w->watcher_next = watchers_head;
        watchers_head->watcher_prev = w;
        watchers_head = w;
    }
}

void Display::rmWatcher(MessageWatcher *w) {
    if( w == watchers_head ) {
        watchers_head = w->watcher_next;
    } else {
        w->watcher_prev->watcher_next = w->watcher_next;
        w->watcher_next->watcher_prev = w->watcher_prev;
    }
}

bool MessageWatcher::checkMessage(const Message &msg) {
    if( code == msg.code ) {
        // Prevent callback from running inside handler of the same callback
        bool flag = hadMessageFlag;
        hadMessageFlag=true;
        if( cb && !flag ) {
            cb(msg);
        }
    }
    return code == msg.code;
}

MessageWatcherNumber::MessageWatcherNumber(Display &d) : MessageWatcher(d, 0x71) {
}

bool MessageWatcherNumber::checkMessage(const Message &msg) {
    if( MessageWatcher::checkMessage(msg) ) {
        val = msg.payload_u32[0];
        return true;
    }
    return false;
}

MessageWatcherString::MessageWatcherString(Display &d) : MessageWatcher(d, 0x70) {
}

bool MessageWatcherString::checkMessage(const Message &msg) {
    if( MessageWatcher::checkMessage(msg) ) {
        val = String((const char*)msg.payload_u8);
        return true;
    }
    return false;
}

bool MessageWatcherSendme::checkMessage(const Message &msg) {
    if( code == msg.code && page_id == msg.payload_u8[0] ) {
        MessageWatcher::checkMessage(msg);
        return true;
    }
    return false;
}

bool MessageWatcherTouchEvent::checkMessage(const Message &msg) {
    if( code == msg.code && page_id == msg.payload_u8[0] && component_id == msg.payload_u8[1] ) {
        MessageWatcher::checkMessage(msg);
        return true;
    }
    return false;
}

};
