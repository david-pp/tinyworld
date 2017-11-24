// Copyright (c) 2017 david++
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __COMMON_EVENT_LOOP_H
#define __COMMON_EVENT_LOOP_H

#include <list>
#include <memory>
#include <functional>
#include <ev.h>

namespace tiny {

typedef std::function<void()> EventCallback;

struct EventHolderBase {
    virtual void called() = 0;
};

typedef std::shared_ptr<EventHolderBase> EventHolderPtr;

template<typename EventType>
struct EventHolder : public EventHolderBase {
    EventHolder(const EventCallback &callback)
            : callback(callback) {}

    void called() override {
        if (callback) callback();
    }

    EventType event;
    EventCallback callback;
};

class EventLoop {
public:
    EventLoop(bool default_ = true);

    static EventLoop *instance() {
        static EventLoop loop;
        return &loop;
    }

    EventLoop &onIdle(const EventCallback &callback);

    EventLoop &onTimer(const EventCallback &callback, ev_tstamp repeat, ev_tstamp after = 0.0);

    void run();

public:
    struct ev_loop *evLoop() { return evloop_; }

private:
    struct ev_loop *evloop_;

    std::list<EventHolderPtr> holders_;
};

} // namespace tiny

#endif // __COMMON_EVENT_LOOP_H