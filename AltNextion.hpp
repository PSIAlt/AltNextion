#pragma once
#ifndef __ALTNEXTION_H__
#define __ALTNEXTION_H__
#include <stdint.h>
#include <HardwareSerial.h>

namespace AltNextion {

#define ALTNETION_MAX_MESSAGE_LEN SERIAL_RX_BUFFER_SIZE
#define MAX_MESSAGE_WATCHERS 10

struct __attribute__ ((packed)) Message {
    uint8_t code;
    union {
        uint8_t payload_u8[16];
        uint16_t payload_u16[8];
        uint32_t payload_u32[4];
    };
    // The "ff ff ff" (do not contain here)
};

// class that manages Nextion single display connection
class Display {
public:
    Display(Stream &stream) : serial(stream) {
    }

    const Message *listen(unsigned long timeout_ms=1000);
    friend class MessageWatcher;
protected:
    void addWatcher(MessageWatcher *w);
    void rmWatcher(MessageWatcher *w);
private:
    Stream &serial;
    union {
        Message msg;
        uint8_t rdbuf[ALTNETION_MAX_MESSAGE_LEN];
    };
    int rdbuf_len=0;
    MessageWatcher *watchers_head = NULL;
    
};

// A class that watches for specific message prefix for you, so you can add listeners
class MessageWatcher {
public:
    MessageWatcher(Display &d, uint8_t code) : _d(d), _code(code) {
        _d.addWatcher(this);
    }
    virtual ~MessageWatcher() {
        _d.rmWatcher(this);
    }

    virtual bool checkMessage(const Message &msg);
private:
    friend class Display;
    Display &_d;
    MessageWatcher *watcher_prev = NULL, *watcher_next = NULL;
    uint8_t _code;
};

class MessageWatcherNumber : public MessageWatcher {
public:
    MessageWatcherNumber(Display &d) : MessageWatcher(d, 0x71) {}
};
class MessageWatcherString : public MessageWatcher {
public:
    MessageWatcherString(Display &d) : MessageWatcher(d, 0x70) {}
};

class MessageWatcherSendme : public MessageWatcher {
public:
    MessageWatcherSendme(Display &d, int page_id=-1) : MessageWatcher(d, 0x66), _page_id(page_id) {}
private:
    int _page_id=-1;
};

// Watch 
class MessageWatcherTouchEvent : public MessageWatcher {
public:
    MessageWatcherTouchEvent(Display &d, int page_id=-1, int component_id=-1) :
        MessageWatcher(d, 0x65), _page_id(page_id), _component_id(component_id) {}
private:
    int _page_id=-1;
    int _component_id=-1;
};


};

#endif
