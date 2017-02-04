#include <iostream>
#include "message_dispatcher.h"
#include "test_msg.pb.h"

void onMsg_LoginRequest_Void()
{
    std::cout << __PRETTY_FUNCTION__  << std::endl;
}

void onMsg_LoginRequest(Cmd::LoginRequest* msg)
{
    std::cout << __PRETTY_FUNCTION__  << std::endl;
    std::cout << msg->DebugString() << std::endl;
}

void onMsg_LoginRequest_1(Cmd::LoginRequest* msg, int arg1)
{
    std::cout << __PRETTY_FUNCTION__  << std::endl;
    std::cout << "arg1:" << arg1 << std::endl;
    std::cout << msg->DebugString() << std::endl;
}

void onMsg_LoginRequest_2(Cmd::LoginRequest* msg, int arg1, int arg2)
{
    std::cout << __PRETTY_FUNCTION__  << std::endl;
    std::cout << "arg1:" << arg1 << std::endl;
    std::cout << "arg2:" << arg2 << std::endl;
    std::cout << msg->DebugString() << std::endl;
}


void test_byname()
{
    std::cout << "name:" << Cmd::LoginRequest::descriptor()->name() << std::endl;
    std::cout << "fullname:" << Cmd::LoginRequest::descriptor()->full_name() << std::endl;

    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    // 打包
    std::string msgbuf;
    ProtobufMsgDispatcherByName::pack(msgbuf, &send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;


    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByName dispatcher1;
        dispatcher1.bind0<Cmd::LoginRequest>(onMsg_LoginRequest_Void);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息为参数
    {
        ProtobufMsgDispatcherByName dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest>(onMsg_LoginRequest);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息+额外1个参数
    {
        ProtobufMsgDispatcherByName dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int>(onMsg_LoginRequest_1);

        dispatcher1.dispatch(header, 1000);
    }
    // 测试消息分发-消息+额外2个参数
    {
        ProtobufMsgDispatcherByName dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int, int>(onMsg_LoginRequest_2);

        dispatcher1.dispatch(header, 1000, 2000);
    }
}

void test_byid_1()
{
    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    // 打包
    std::string msgbuf;
    ProtobufMsgDispatcherByOneID::pack(msgbuf, &send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByOneID dispatcher1;
        dispatcher1.bind0<Cmd::LoginRequest>(onMsg_LoginRequest_Void);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息为参数
    {
        ProtobufMsgDispatcherByOneID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest>(onMsg_LoginRequest);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息+额外1个参数
    {
        ProtobufMsgDispatcherByOneID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int>(onMsg_LoginRequest_1);

        dispatcher1.dispatch(header, 1000);
    }
    // 测试消息分发-消息+额外2个参数
    {
        ProtobufMsgDispatcherByOneID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int, int>(onMsg_LoginRequest_2);
        dispatcher1.dispatch(header, 1000, 2000);
    }
}

void test_byid_2()
{
    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    // 打包
    std::string msgbuf;
    ProtobufMsgDispatcherByTwoID::pack(msgbuf, &send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByTwoID dispatcher1;
        dispatcher1.bind0<Cmd::LoginRequest>(onMsg_LoginRequest_Void);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息为参数
    {
        ProtobufMsgDispatcherByTwoID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest>(onMsg_LoginRequest);

        dispatcher1.dispatch(header);
    }
    // 测试消息分发-消息+额外1个参数
    {
        ProtobufMsgDispatcherByTwoID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int>(onMsg_LoginRequest_1);

        dispatcher1.dispatch(header, 1000);
    }
    // 测试消息分发-消息+额外2个参数
    {
        ProtobufMsgDispatcherByTwoID dispatcher1;
        dispatcher1.bind<Cmd::LoginRequest, int, int>(onMsg_LoginRequest_2);
        dispatcher1.dispatch(header, 1000, 2000);
    }
}

int main(int argc, const char* argv[])
{
//    test_byname();
//    test_byid_1();
    test_byid_2();
}