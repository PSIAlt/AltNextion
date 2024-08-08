#pragma once
#ifndef __ALTNEXTION_H__
#define __ALTNEXTION_H__
#include <stdint.h>

namespace AltNextion {

#define ALTNETION_MAX_MESSAGE_LEN SERIAL_RX_BUFFER_SIZE
#define MAX_MESSAGE_WATCHERS 10

struct __attribute__ ((packed)) Message {
    uint32_t len;
    uint8_t code;
    union {
        uint8_t payload_u8[16];
        uint16_t payload_u16[8];
        uint32_t payload_u32[4];
    };
    // The "ff ff ff" (do not contain here)
};
class MessageWatcher;

// class that manages Nextion single display connection
class Display {
public:
    Display(Stream &stream) : serial(stream) {
    }

    bool listen(long timeout_ms=1000);
    bool getComponentValue(const char *name, uint32_t &val);
    bool getComponentValue(const char *name, String &val);
    bool sendCommand(const char *cmd, int len=-1);
    bool waitAnswerWithWatcher(MessageWatcher &w);

    friend class MessageWatcher;
protected:
    void dispatchEvent();
    void addWatcher(MessageWatcher *w);
    void rmWatcher(MessageWatcher *w);
private:
    Stream &serial;
    union {
        Message msg;
        uint8_t rdbuf[ALTNETION_MAX_MESSAGE_LEN];
    };
    int rdbuf_len=0, endcount=0;
    MessageWatcher *watchers_head = NULL;
    
};

// A class that watches for specific message prefix for you, so you can add listeners
typedef void (*watcherCallbackPtr)(const Message &msg);
class MessageWatcher {
public:
    MessageWatcher(Display &_d, uint8_t _code) : d(_d), code(_code) {
        d.addWatcher(this);
    }
    virtual ~MessageWatcher() {
        d.rmWatcher(this);
    }
    void setCallback(watcherCallbackPtr _cb) { cb = _cb; }

    virtual bool checkMessage(const Message &msg);
    bool hadMessage() const { return hadMessageFlag; }
private:
    friend class Display;
    Display &d;
    MessageWatcher *watcher_prev = NULL, *watcher_next = NULL;
    watcherCallbackPtr cb;
protected:
    uint8_t code;
    bool hadMessageFlag = false;
};

class MessageWatcherNumber : public MessageWatcher {
public:
    MessageWatcherNumber(Display &d);

    bool checkMessage(const Message &msg);
    uint32_t getValue() { return val; }
private:
    uint32_t val=-1;
};

class MessageWatcherString : public MessageWatcher {
public:
    MessageWatcherString(Display &d);

    bool checkMessage(const Message &msg);
    const String &getValue() { return val; }
private:
    String val;
};

class MessageWatcherSendme : public MessageWatcher {
public:
    MessageWatcherSendme(Display &d, int _page_id=-1) : MessageWatcher(d, 0x66), page_id(_page_id) {}

    bool checkMessage(const Message &msg);
private:
    int page_id=-1;
};

// Watch 
class MessageWatcherTouchEvent : public MessageWatcher {
public:
    MessageWatcherTouchEvent(Display &d, int _page_id=-1, int _component_id=-1) :
        MessageWatcher(d, 0x65), page_id(_page_id), component_id(_component_id) {}

    bool checkMessage(const Message &msg);
private:
    int page_id=-1;
    int component_id=-1;
};


};

#endif
