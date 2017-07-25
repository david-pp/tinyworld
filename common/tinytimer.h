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

#ifndef TINYWORLD_TINYTIMER_H
#define TINYWORLD_TINYTIMER_H

#include <functional>
#include <vector>
#include <memory>

namespace tiny {

    class TimerEvent {
    public:
        typedef std::function<void()> Callback;

        TimerEvent(uint32_t timepoint, const Callback &cb, uint32_t interval = 300)
                : timepoint_(timepoint), interval_(interval), callback_(cb) {}

        bool run(uint32_t now) {
            if (!runtime_) {
                if (now >= timepoint_ && now <= (timepoint_ + interval_)) {
                    callback_();
                    runtime_ = now;
                    return true;
                }
            }
            return false;
        }

    private:
        uint32_t timepoint_ = 0;
        uint32_t interval_ = 0;
        uint32_t runtime_ = 0;

        Callback callback_;
    };

    class TimerEvents {
    public:
        void at(uint32_t timepoint, const TimerEvent::Callback &callback, uint32_t interval = 300) {
            auto event = std::make_shared<TimerEvent>(timepoint, callback, interval);
            if (event) {
                events_.push_back(event);
            }
        }

        void run(uint32_t now) {
            for (auto event : events_) {
                if (event)
                    event->run(now);
            }
        }

    private:
        std::vector<std::shared_ptr<TimerEvent>> events_;
    };

} // end namespace tiny

#endif //TINYWORLD_TINYTIMER_H
