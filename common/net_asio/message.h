#ifndef _NET_ASIO_MESSAGE_H
#define _NET_ASIO_MESSAGE_H

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
//      6bytes       mh.size
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
		     << " reserved=" << reserved;
		return oss.str();
	}

	uint32 size:24;            // message body size
	uint32 zip:1;              // message is zipped
	uint32 encrypt:1;          // message is encrypted
	uint32 checksum:1;         // checksum
	uint32 reserved:5;

	union {
		struct {
			uint8 type_first;  // first type 
			uint8 type_second; // second type
		};
		uint16 type;           // message type;
	};
};

// 消息类型
struct MessageType
{
	union {
		struct {
			uint8 type_first;  // first type 
			uint8 type_second; // second type
		};
		uint16 type;           // message type;
	};
};

#pragma pack()

//
// 消息类型的工具函数
//
inline uint8 MSG_TYPE_1(uint16 type)
{
	MessageType mt;
	mt.type = type;
	return mt.type_first;
}

inline uint8 MSG_TYPE_2(uint16 type)
{
	MessageType mt;
	mt.type = type;
	return mt.type_second;
}

inline uint16 MAKE_MSG_TYPE(uint8 type1, uint8 type2) 
{
	MessageType mt;
	mt.type_first = type1;
	mt.type_second = type2;
	return mt.type;
}


#endif // _COMMON_MESSAGE_HEADER_H