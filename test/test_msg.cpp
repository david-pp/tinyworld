#include <iostream>
#include "message_dispatcher.h"
#include "test_msg.pb.h"


struct Object
{
    int v = 314;

    void doMsg(const Cmd::LoginRequest &msg)
    {
        std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
        std::cout << msg.DebugString() << std::endl;
        std::cout <<"v: " << v << std::endl;
    }
};

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
    ProtobufMsgHandlerBase::packByName(msgbuf, send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    Object obj;

    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByName<int, Object *> d;
        d.on_void<Cmd::LoginRequest>([]() {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "args: void" << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-多参数
    {
        ProtobufMsgDispatcherByName<int, Object *> d;
        d.on<Cmd::LoginRequest>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << msg.DebugString() << std::endl;
            std::cout << "arg1:" << a1 << std::endl;
            std::cout << "arg2:" << a2->v << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-bind
    {
        ProtobufMsgDispatcherByName<> d;
        d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));

        d.dispatch(header);
    }
}


void test_byid1()
{
    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    // 打包
    std::string msgbuf;
    ProtobufMsgHandlerBase::packByID1(msgbuf, send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    Object obj;

    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByID1<int, Object *> d;
        d.on_void<Cmd::LoginRequest>([]() {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "args: void" << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-多参数
    {
        ProtobufMsgDispatcherByID1<int, Object *> d;
        d.on<Cmd::LoginRequest>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << msg.DebugString() << std::endl;
            std::cout << "arg1:" << a1 << std::endl;
            std::cout << "arg2:" << a2->v << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-bind
    {
        ProtobufMsgDispatcherByID1<> d;
        d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));

        d.dispatch(header);
    }
}


void test_byid2()
{
    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    // 打包
    std::string msgbuf;
    ProtobufMsgHandlerBase::packByID2(msgbuf, send);

    MessageHeader* header = (MessageHeader*)msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    Object obj;

    // 测试消息分发-无参数
    {
        ProtobufMsgDispatcherByID2<int, Object *> d;
        d.on_void<Cmd::LoginRequest>([]() {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "args: void" << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-多参数
    {
        ProtobufMsgDispatcherByID2<int, Object *> d;
        d.on<Cmd::LoginRequest>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
            std::cout << msg.DebugString() << std::endl;
            std::cout << "arg1:" << a1 << std::endl;
            std::cout << "arg2:" << a2->v << std::endl;
        });

        d.dispatch(header, 100, &obj);
    }

    // 测试消息分发-bind
    {
        ProtobufMsgDispatcherByID2<> d;
        d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));

        d.dispatch(header);
    }
}

int main(int argc, const char* argv[])
{
    test_byname();
    test_byid1();
    test_byid2();
}