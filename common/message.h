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

#ifndef _COMMON_MESSAGE_H
#define _COMMON_MESSAGE_H

#include "tinyworld.h"

#include <iostream>
#include <sstream>


// MessageHeader
// 
// +-------------+-------------+
// |MessageHeader| Body        |
// +-------------+-------------+
//      4bytes       mh.size
//

#pragma pack(1)

struct MessageHeader {
    MessageHeader() {
        size = 0;
        zip = 0;
        encrypt = 0;
        checksum = 0;
        reserved = 0;
    }

    // whole message size
    uint32_t msgsize() { return sizeof(MessageHeader) + size; }

    // message bodysize
    uint32_t bodysize() { return size; }

    // TODO: debug only
    std::string dumphex() {
        std::ostringstream oss;
        uint8_t *bin = (uint8_t *) this;
        for (size_t i = 0; i < sizeof(MessageHeader); ++i)
            oss << (int) bin[i] << ",";

        oss << " | size=" << size
            << " zip=" << zip
            << " encrypt=" << encrypt
            << " checksum=" << checksum
            << " type_is_name=" << type_is_name;
        return oss.str();
    }

    uint32_t size:24;            // message body size
    uint32_t zip:1;              // message is zipped
    uint32_t encrypt:1;          // message is encrypted
    uint32_t checksum:1;         // undefined
    uint32_t type_is_name:1;     // type is name
    uint32_t reserved:4;         // undefined

    union {
        struct {
            uint8_t type_first;  // first type
            uint8_t type_second; // second type
        };
        uint16_t type;           // message type;
        uint16_t type_len;       // message type length(when type_is_name=1)
    };
};

#pragma pack()

#define MSG_TYPE_1(type) ((type) & 0xff)
#define MSG_TYPE_2(type) ((type) >> 8)
#define MAKE_MSG_TYPE(type1, type2) (((type2) << 8) + (type1))

//inline uint8 MSG_TYPE_1(const uint16 type)
//{
//    MessageType mt;
//    mt.type = type;
//    return mt.type_first;
//}
//
//inline uint8 MSG_TYPE_2(const uint16 type)
//{
//    MessageType mt;
//    mt.type = type;
//    return mt.type_second;
//}
//
//inline const uint16 MAKE_MSG_TYPE(const uint8 type1, const uint8 type2)
//{
//    MessageType mt;
//    mt.type_first = type1;
//    mt.type_second = type2;
//    return mt.type;
//}

#endif // _COMMON_MESSAGE_H