#include <Arduino.h>

#include "AltNextion.hpp"

namespace AltNextion {
const uint8_t endbyte = 0xff;

bool Display::listen(long timeout_ms) {
    unsigned long last_ms = millis();
    const int delay_ms = 10;
    while( serial.available()>0 & timeout_ms>delay_ms ){
        delay(delay_ms);
        unsigned long m = millis();
        timeout_ms -= (m-last_ms);
        last_ms=m;
        serial.setTimeout( timeout_ms > delay_ms ? timeout_ms : delay_ms );
        size_t rd = serial.readBytesUntil(endbyte, &rdbuf[rdbuf_len], sizeof(rdbuf)-rdbuf_len);
        for(int i=rdbuf_len; i<(rdbuf_len+rd); i++) {
            if( rdbuf[i] == endbyte )
                endcount++;
        }
        rdbuf_len += rd;
        if( rdbuf_len >= sizeof(rdbuf) )
            rdbuf_len = sizeof(rdbuf)-1;
        if( endcount > 3 )
            abort(); //omg somethign wrong happend
        if( endcount == 3 ) {
            msg.len = rdbuf_len - endcount;
            msg.payload_u8[msg.len-1] = 0;

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
    MessageWatcher *w = watchers_head;
    while( w ) {
        w->checkMessage(msg);
        w = w->watcher_next;
    }
    endcount = 0;
    rdbuf_len = 0;
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
    if( MessageWatcher::checkMessage(msg ) ) {
        val = msg.payload_u32[0];
        return true;
    }
    return false;
}

MessageWatcherString::MessageWatcherString(Display &d) : MessageWatcher(d, 0x70) {
}

bool MessageWatcherString::checkMessage(const Message &msg) {
    if( MessageWatcher::checkMessage(msg ) ) {
        val = String((const char*)msg.payload_u8); //-1 for the code
        return true;
    }
    return false;
}

bool MessageWatcherSendme::checkMessage(const Message &msg) {
    if( code == msg.code && page_id == msg.payload_u8[0] ) {
        MessageWatcher::checkMessage(msg );
        return true;
    }
    return false;
}

bool MessageWatcherTouchEvent::checkMessage(const Message &msg) {
    if( code == msg.code && page_id == msg.payload_u8[0] && component_id == msg.payload_u8[1] ) {
        MessageWatcher::checkMessage(msg );
        return true;
    }
    return false;
}

};
