#include <iostream>
#include "message.h"
#include "message_dispatcher.h"
#include "protos/account.pb.h"

void onRegisterWithNamePasswordVoid()
{
	std::cout << __PRETTY_FUNCTION__  << std::endl;
}

void onRegisterWithNamePassword(Cmd::Account::RegisterWithNamePasswordRequest* msg)
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
}

void onRegisterWithNamePasswordInt(Cmd::Account::RegisterWithNamePasswordRequest* msg, int arg)
{
	std::cout << __PRETTY_FUNCTION__  << std::endl;
	std::cout << arg << std::endl;
	std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
}



struct RegisterWithDeviceIDHandler
{
	void operator () (Cmd::Account::RegisterWithDeviceIDRequest* msg)
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
		std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
	}
};


struct XXX
{
	void doMsg2(Cmd::Account::RegisterWithDeviceIDRequest* msg)
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
		std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
	}

	void doMsg3(Cmd::Account::RegisterWithDeviceIDRequest* msg, int arg)
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
		std::cout << arg << std::endl;
		std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
	}
};

XXX xxx;

void onRegisterWithNamePasswordXXX(Cmd::Account::RegisterWithNamePasswordRequest* msg, XXX* arg1, int arg2)
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	std::cout << arg1 << std::endl;
	std::cout << arg2 << std::endl;
	std::cout << msg->GetTypeName() << ":\n" << msg->DebugString() << std::endl;
}



int main(int argc, const char* argv[])
{
	std::string msg1;
	uint16 msgtype1 = MAKE_MSG_TYPE(Cmd::Account::RegisterWithNamePasswordRequest::TYPE1, Cmd::Account::RegisterWithNamePasswordRequest::TYPE2);
	{
		Cmd::Account::RegisterWithNamePasswordRequest  request;
		request.set_username("david");
		request.set_passwd("123456");
		request.SerializeToString(&msg1);
		std::cout << request.DebugString() << std::endl;
	}

	std::string msg2;
	uint16 msgtype2 = MAKE_MSG_TYPE(Cmd::Account::RegisterWithDeviceIDRequest::TYPE1, Cmd::Account::RegisterWithDeviceIDRequest::TYPE2);
	{
		Cmd::Account::RegisterWithDeviceIDRequest  request;
		request.set_device_id("XXXXXX-22222");
		request.set_device_type("iphone");
		request.SerializeToString(&msg2);
		std::cout << request.DebugString() << std::endl;

		//boost::bind(&XXX::doMsg2, xxx, _1)(&request);
	}

	// 消息处理函数没有参数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind0<Cmd::Account::RegisterWithNamePasswordRequest>(onRegisterWithNamePasswordVoid);
		dispatcher.dispatch(msgtype1, (void*)msg1.data(), msg1.size());
	}
	// 消息处理函数只有Message参数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithNamePasswordRequest>(onRegisterWithNamePassword);
		dispatcher.dispatch(msgtype1, (void*)msg1.data(), msg1.size());
	}
	// 消息处理函数有1个额外参数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithNamePasswordRequest, int>(onRegisterWithNamePasswordInt);
		dispatcher.dispatch(msgtype1, (void*)msg1.data(), msg1.size(),12345);
	}
	// 消息处理函数有两个额外参数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithNamePasswordRequest, XXX*, int>(onRegisterWithNamePasswordXXX);
		dispatcher.dispatch(msgtype1, (void*)msg1.data(), msg1.size(), &xxx, 23456);
	}

	// 函数对象
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithDeviceIDRequest>(RegisterWithDeviceIDHandler());
		dispatcher.dispatch(msgtype2, (void*)msg2.data(), msg2.size());
	}

	// 成员函数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithDeviceIDRequest>(boost::bind(&XXX::doMsg2, xxx, _1));
		dispatcher.dispatch(msgtype2, (void*)msg2.data(), msg2.size());
	}
	// 成员函数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithDeviceIDRequest, int>(
			boost::bind(&XXX::doMsg3, xxx, _1, _2));
		dispatcher.dispatch(msgtype2, (void*)msg2.data(), msg2.size(), 2345);
	}
	// 成员函数
	{
		ProtobufMsgDispatcher dispatcher;
		dispatcher.bind<Cmd::Account::RegisterWithDeviceIDRequest>(
			boost::bind(&XXX::doMsg3, xxx, _1, 1111));
		dispatcher.dispatch(msgtype2, (void*)msg2.data(), msg2.size());
	}
}