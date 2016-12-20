#include <iostream>
#include <netinet/in.h>

#include "net_asio/message.h"

const int endian = 1;

#define is_bigendian() ( (*(char*) &endian) == 0 )

#define is_littlendbian() ( (*(char*) &endian) == 1 )

int main()
{
	if (is_littlendbian())
	{
		std::cout << "little endian" << std::endl;
	}
	else
	{
		std::cout << "little endian" << std::endl;
	}

	// 本地字节序
	uint32 bodysize = 0x12345678;
	MessageHeader mh;
	mh.size = bodysize;

	{
		uint8* bin =(uint8*)&mh;

		std::cout << "[0] - " << std::hex << (int)bin[0] << std::endl;
		std::cout << "[1] - " << std::hex << (int)bin[1] << std::endl;
		std::cout << "[2] - " << std::hex << (int)bin[2] << std::endl;
		std::cout << "[3] - " << std::hex << (int)bin[3] << std::endl;
		std::cout << std::hex << mh.size << std::endl;
	}


	{
		uint8* bin =(uint8*)&bodysize;

		std::cout << "[0] - " << std::hex << (int)bin[0] << std::endl;
		std::cout << "[1] - " << std::hex << (int)bin[1] << std::endl;
		std::cout << "[2] - " << std::hex << (int)bin[2] << std::endl;
		std::cout << "[3] - " << std::hex << (int)bin[3] << std::endl;
		std::cout << std::hex << mh.size << std::endl;
	}

	// 网络字节序(Big Endian)
	//uint32 netbodysize = htonl(bodysize);
	{
		MessageHeader mh;
		mh.size = htonl(bodysize);
		uint8* bin =(uint8*)&mh;

		std::cout << "[0] - " << std::hex << (int)bin[0] << std::endl;
		std::cout << "[1] - " << std::hex << (int)bin[1] << std::endl;
		std::cout << "[2] - " << std::hex << (int)bin[2] << std::endl;
		std::cout << "[3] - " << std::hex << (int)bin[3] << std::endl;
	}

}