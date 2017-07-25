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

#ifndef TINYWORLD_TINYFSM_H
#define TINYWORLD_TINYFSM_H

#include <map>
#include "tinytimer.h"

namespace tiny {

    // FSM/TimerFSM is a simple finite state machine library for C++.
    //
    // Methods:
    //  fsm.state() - return current state
    //  fsm.is(s)   - return true if state s is the current state
    //  fsm.can(ev) - return true if transition t can occur from the current state
    //
    //  fsm.transit(ev) - transition to a different state by event
    //
    // Usage:
    //  1. State & Event Enumeration
    //    enum StateEnum {
    //        solid,
    //        liquid,
    //        gas
    //    };
    //
    //    enum EventEnum {
    //        melt,
    //        freeze,
    //        vaporize,
    //        condense,
    //    };
    //
    //  2. Construction
    //     TimerFSM fsm(solid);
    //         fsm.addTransition(solid, liquid, melt);
    //         fsm.addTransition(liquid, gas, vaporize);
    //         fsm.addTransition(gas, liquid, timer_1, now+5);
    //
    //     fsm.onBefore(melt, []() {
    //         std::cout << "Before Melt..." << std::endl;
    //     });
    //
    //     fsm.onAfter(melt, []() {
    //          std::cout << "After Melt..." << std::endl;
    //     });
    //
    //     fsm.onLeave(solid, []() {
    //          std::cout << "Leave frome Solid ..." << std::endl;
    //     });
    //
    //     fsm.onEnter(liquid, []() {
    //          std::cout << "Enter to liquid..." << std::endl;
    //     });
    //
    //  3. Trigger
    //    fsm.transit(melt);
    //    fsm.transit(vaporize);
    //
    // idea from : https://github.com/jakesgordon/javascript-state-machine
    //
    class FSM {
    public:
        typedef std::function<void()> EventAction;
        typedef std::function<void()> StateAction;
        typedef std::function<void(uint32_t, uint32_t, uint32_t)> TransitionAction;

        FSM(uint32_t initstate) : current_(initstate) {}

        uint32_t state() const { return current_; }

        bool is(uint32_t state) const {
            return current_ == state;
        }

        bool can(uint32_t event) {
            return transitions_[current_].find(event) != transitions_[current_].end();
        }

        bool transit(uint32_t event) {
            if (!can(event))
                return false;

            // on-before
            if (event_actions_[event].first)
                event_actions_[event].first();

            // on-leave
            if (state_actions_[current_].second)
                state_actions_[current_].second();

            const uint32_t from = current_;
            current_ = transitions_[current_][event];

            // on-enter
            if (state_actions_[current_].first)
                state_actions_[current_].first();

            // on-after
            if (event_actions_[event].second)
                event_actions_[event].second();

            if (action_transition_)
                action_transition_(from, current_, event);

            return true;
        }

    public:
        bool addTransition(uint32_t from, uint32_t to, uint32_t event) {
            transitions_[from][event] = to;
            return true;
        }

        void onTransition(const TransitionAction &callback) {
            action_transition_ = callback;
        }

        //
        // Event Actions(Before/After)
        //
        void onBefore(uint32_t event, const EventAction &action) {
            event_actions_[event].first = action;
        }

        void onAfter(uint32_t event, const EventAction &action) {
            event_actions_[event].second = action;
        }

        //
        // State Actions(Enter/Leave)
        //
        void onEnter(uint32_t state, const StateAction &action) {
            state_actions_[state].first = action;
        }

        void onLeave(uint32_t state, const StateAction &action) {
            state_actions_[state].second = action;
        }

    private:
        // current state
        uint32_t current_ = 0;
        // <state, <event, state>>
        std::map<uint32_t, std::map<uint32_t, uint32_t >> transitions_;
        // <state, <enter, leave>>
        std::map<uint32_t, std::pair<StateAction, StateAction>> state_actions_;
        // <event, <before, after>
        std::map<uint32_t, std::pair<EventAction, EventAction>> event_actions_;

        // transition callback
        TransitionAction action_transition_;
    };

    class TimerFSM : public FSM {
    public:
        using FSM::FSM;

        void run(uint32_t now) {
            for (auto timer : timers_) {
                timer.second->run(now);
            }
        }

        bool addTimerTransition(uint32_t from, uint32_t to, uint32_t event,
                                  uint32_t timepoint, uint32_t interval = 300) {
            auto timer = std::make_shared<TimerEvent>(timepoint, [event, this]() {
                this->transit(event);
            }, interval);
            if (timer) {
                timers_[event] = timer;
            }

            return addTransition(from, to, event);
        }

    private:
        std::map<uint32_t, std::shared_ptr<TimerEvent>> timers_;
    };

} // end namespace tiny

#endif //TINYWORLD_TINYFSM_H
