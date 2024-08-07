#include <Arduino.h>

#include "AltNextion.hpp"

namespace AltNextion {
const Message *Display::listen(unsigned long timeout_ms=1000) {

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
    return _code == msg.code;
}

};
