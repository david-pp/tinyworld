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

#ifndef TINYWORLD_MESSAGE_HELPER_H
#define TINYWORLD_MESSAGE_HELPER_H

#include <exception>
#include <string>
#include <google/protobuf/message.h>

//
// Dispatcher Exception
//
class MsgDispatcherException : public std::exception {
public:
    MsgDispatcherException(const std::string &msg)
            : errmsg_(msg) {}

    virtual const char *what() const noexcept {
        return errmsg_.c_str();
    }

private:
    std::string errmsg_;
};

//
// Message Name
//
template<typename MsgT, int>
struct MessageNameImpl;


// Primary Template
template<typename MsgT>
struct MessageName {
    static std::string value() {
        return MessageNameImpl<MsgT,
                std::is_base_of<google::protobuf::Message, MsgT>::value ? 0 : 1>::value();
    }
};

// This is for Protobuf Generated Class
template<typename MsgT>
struct MessageNameImpl<MsgT, 0> {
    static std::string value() {
        return MsgT::descriptor()->full_name();
    }
};

// This is for UserDefined Class
template<typename MsgT>
struct MessageNameImpl<MsgT, 1> {
    static std::string value() {
        return MsgT::msgName();
    }
};

//
// Message Type Code
//
template<typename MsgT>
struct MessageTypeCode {
    static uint16 value() {
        return MsgT::msgType();
    }
};

//
// Helper Macros
//
//  DECLARE_MESSAGE(MsgT)                  : MsgT -> "MsgT"
//  DECLARE_MESSAGE_BY_NAME(MsgT, "msg")   : MsgT -> "msg"
//  DECLARE_MESSAGE_BY_TYPE(MsgT, 1024)    : MsgT -> 1024
//  DECLARE_MESSAGE_BY_TYPE2(MsgT, 20, 30) : MsgT -> (20, 30)
//
#define DECLARE_MESSAGE(MsgType) \
    template <> struct MessageName<MsgType> { \
        static std::string value() { return #MsgType; } }

#define DECLARE_MESSAGE_BY_NAME(MsgType, name) \
    template <> struct MessageName<MsgType> { \
        static std::string value() { return name; } }

#define DECLARE_MESSAGE_BY_TYPE(MsgType, type) \
    template <> struct MessageTypeCode<MsgType> { \
        static uint16 value() { return type; } }

#define DECLARE_MESSAGE_BY_TYPE2(MsgType, type1, type2) \
    template <> struct MessageTypeCode<MsgType> { \
        static uint16 value() { return MAKE_MSG_TYPE(type1, type2); } }


#endif //TINYWORLD_MESSAGE_HELPER_H
