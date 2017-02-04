#ifndef _COMMON_MESSAGE_H
#define _COMMON_MESSAGE_H

#include "tinyworld.h"
#include <iostream>
#include <sstream>

////////////////////////////////////////////////////////////
//
// MessageHeader
// 
// +-------------+-------------+
// |MessageHeader| Body        |
// +-------------+-------------+
//      4bytes       mh.size
//
////////////////////////////////////////////////////////////

#pragma pack(1)

struct MessageHeader
{
	MessageHeader()
	{
		size = 0;
		zip = 0;
		encrypt = 0;
		checksum = 0;
		reserved = 0;
	}

    // 整个消息的大小
    size_t msgsize() { return sizeof(MessageHeader) + size; }

    // 消息体的大小
    size_t bodysize() { return size; }

    std::string dumphex()
    {
        std::ostringstream oss;
        uint8* bin = (uint8*)this;
        for (size_t i = 0; i < sizeof(MessageHeader); ++ i)
            oss << (int)bin[i] << ",";

        oss << " | size=" << size
            << " zip=" << zip
            << " encrypt=" << encrypt
            << " checksum=" << checksum
            << " type_is_name=" << type_is_name;
        return oss.str();
    }

    uint32 size:24;            // message body size
	uint32 zip:1;              // message is zipped
	uint32 encrypt:1;          // message is encrypted
	uint32 checksum:1;         // undefined
    uint32 type_is_name:1;     // type is name
	uint32 reserved:4;         // undefined

	union {
		struct {
			uint8 type_first;  // first type 
			uint8 type_second; // second type
		};
		uint16 type;           // message type;

        uint16 type_len;       // message type length(when type_is_name=1)
	};
};


#pragma pack()

//
// 消息类型的工具函数
//
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