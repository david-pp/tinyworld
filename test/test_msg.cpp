#include <iostream>
#include "message_dispatcher.h"
#include "test_msg.pb.h"


struct Object {
    int v = 314;

    void doMsg(const Cmd::LoginRequest &msg) {
        std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
        std::cout << msg.DebugString() << std::endl;
        std::cout << "v: " << v << std::endl;
    }
};

void test_byname() {
    std::cout << "name:" << Cmd::LoginRequest::descriptor()->name() << std::endl;
    std::cout << "fullname:" << Cmd::LoginRequest::descriptor()->full_name() << std::endl;

    // 准备消息
    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.DebugString() << std::endl;

    MessageBuffer buf;
    buf.writeByName(send);

    // 打包
    std::string &msgbuf = buf.str();
//    ProtobufMsgDispatcherByName<>::pack(msgbuf, send);
//    MessageBuffer::packMsgByName(msgbuf, Cmd::LoginRequest::descriptor()->full_name(), send);

    MessageHeader *header = (MessageHeader *) msgbuf.data();
    std::cout << header->dumphex() << std::endl;

    std::cout << "-----------------" << std::endl;

    Object obj;

    try {
        // 测试消息分发-无参数/无返回值
        {
            ProtobufMsgDispatcherByName<int, Object *> d;
            d.on_void<Cmd::LoginRequest>([]() {
                std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
                std::cout << "args: void" << std::endl;
            });

            d.dispatch(header, 100, &obj);
        }

        // 测试消息分发-无参数/有返回值
        {
            ProtobufMsgDispatcherByName<int, Object *> d;
            d.on_void<Cmd::LoginRequest, Cmd::LoginReply>([]() {
                std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
                std::cout << "args: void" << std::endl;

                Cmd::LoginReply reply;
                reply.set_info("good!");
                return reply;
            });

            d.on<Cmd::LoginReply>([](const Cmd::LoginReply &msg, int a1, Object *a2) {
                std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
                std::cout << "arg1:" << a1 << std::endl;
                std::cout << "arg2:" << a2->v << std::endl;
            });

            auto reply = d.dispatch(header, 100, &obj);
            if (reply) {
                d.dispatch((MessageHeader *) reply->data(), 200, &obj);
            }
        }

        // 测试消息分发-多参数
        {
            ProtobufMsgDispatcherByName<int, Object *> d;

            d.on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
                std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
                std::cout << "arg1:" << a1 << std::endl;
                std::cout << "arg2:" << a2->v << std::endl;

                Cmd::LoginReply reply;
                reply.set_info("cool!");
                return reply;
            });

            d.on<Cmd::LoginReply>([](const Cmd::LoginReply &msg, int a1, Object *a2) {
                std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
                std::cout << "arg1:" << a1 << std::endl;
                std::cout << "arg2:" << a2->v << std::endl;
            });

            auto replybin = d.dispatch(header, 100, &obj);
            if (replybin) {
                d.dispatch((MessageHeader *) replybin->data(), 200, &obj);
            }
        }

        // 测试消息分发-bind
        {
            ProtobufMsgDispatcherByName<> d;
            d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));

            d.dispatch(header);
        }
    }
    catch (std::exception &err) {
        std::cout << "[dispatche] " << err.what() << std::endl;
    }
}


//void test_byid1()
//{
//    // 准备消息
//    Cmd::LoginRequest send;
//    send.set_id(12345);
//    send.set_password("passwd");
//    send.set_type(20);
//    std::cout << send.DebugString() << std::endl;
//
//    // 打包
//    std::string msgbuf;
//    ProtobufMsgHandlerBase::packByID1(msgbuf, send);
//
//    MessageHeader* header = (MessageHeader*)msgbuf.data();
//    std::cout << header->dumphex() << std::endl;
//
//    std::cout << "-----------------" << std::endl;
//
//    Object obj;
//
//    // 测试消息分发-无参数
//    {
//        ProtobufMsgDispatcherByID1<int, Object *> d;
//        d.on_void<Cmd::LoginRequest>([]() {
//            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
//            std::cout << "args: void" << std::endl;
//        });
//
//        d.dispatch(header, 100, &obj);
//    }
//
//    // 测试消息分发-多参数
//    {
//        ProtobufMsgDispatcherByID1<int, Object *> d;
//        d.on<Cmd::LoginRequest>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
//            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
//            std::cout << msg.DebugString() << std::endl;
//            std::cout << "arg1:" << a1 << std::endl;
//            std::cout << "arg2:" << a2->v << std::endl;
//        });
//
//        d.dispatch(header, 100, &obj);
//    }
//
//    // 测试消息分发-bind
//    {
//        ProtobufMsgDispatcherByID1<> d;
//        d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));
//
//        d.dispatch(header);
//    }
//}
//
//
//void test_byid2()
//{
//    // 准备消息
//    Cmd::LoginRequest send;
//    send.set_id(12345);
//    send.set_password("passwd");
//    send.set_type(20);
//    std::cout << send.DebugString() << std::endl;
//
//    // 打包
//    std::string msgbuf;
//    ProtobufMsgHandlerBase::packByID2(msgbuf, send);
//
//    MessageHeader* header = (MessageHeader*)msgbuf.data();
//    std::cout << header->dumphex() << std::endl;
//
//    std::cout << "-----------------" << std::endl;
//
//    Object obj;
//
//    // 测试消息分发-无参数
//    {
//        ProtobufMsgDispatcherByID2<int, Object *> d;
//        d.on_void<Cmd::LoginRequest>([]() {
//            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
//            std::cout << "args: void" << std::endl;
//        });
//
//        d.dispatch(header, 100, &obj);
//    }
//
//    // 测试消息分发-多参数
//    {
//        ProtobufMsgDispatcherByID2<int, Object *> d;
//        d.on<Cmd::LoginRequest>([](const Cmd::LoginRequest &msg, int a1, Object *a2) {
//            std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
//            std::cout << msg.DebugString() << std::endl;
//            std::cout << "arg1:" << a1 << std::endl;
//            std::cout << "arg2:" << a2->v << std::endl;
//        });
//
//        d.dispatch(header, 100, &obj);
//    }
//
//    // 测试消息分发-bind
//    {
//        ProtobufMsgDispatcherByID2<> d;
//        d.on<Cmd::LoginRequest>(std::bind(&Object::doMsg, obj, std::placeholders::_1));
//
//        d.dispatch(header);
//    }
//}


#include "tinyserializer_proto.h"

void test_1() {
    std::cout << "--------" << __PRETTY_FUNCTION__ << std::endl;

    Cmd::LoginRequest send;
    send.set_id(12345);
    send.set_password("passwd");
    send.set_type(20);
    std::cout << send.ShortDebugString() << std::endl;


    MessageBuffer msgbuf;
    msgbuf.writeByName(send);

    hexdump(msgbuf.str());

    MessageDispatcher<> dispatcher;
    dispatcher
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &request) {
                std::cout << "----" << __PRETTY_FUNCTION__ << std::endl;
                std::cout << request.DebugString() << std::endl;
                Cmd::LoginReply reply;
                reply.set_info("hello");
                return reply;
            })
            .on<Cmd::LoginReply>([](const Cmd::LoginReply &msg) {
                std::cout << "----" << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
            });

    MessageBufferPtr reply = dispatcher.dispatch(msgbuf.str());
    if (reply) {
        if (!dispatcher.dispatch(reply->str())) {
            std::cout << "---- over" << std::endl;
        }
    }
}

struct LoginReq {

    int type = 1024;
    std::string password = "123467";


    std::string serialize() const {
        Cmd::LoginRequest proto;
        proto.set_id(12345);
        proto.set_password("passwd");
        proto.set_type(20);
        return proto.SerializeAsString();
    }

    bool deserialize(const std::string &data) {
        Cmd::LoginRequest proto;
        if (proto.ParseFromString(data)) {
            type = proto.type();
            password = proto.password();
            return true;
        }
        return false;
    }

    void dump() const {
        std::cout << "LoginReq:" << type << " - " << password << std::endl;
    }
};

//template <>
//struct MessageName<LoginReq> {
//    static std::string value() {
//        return "req";
//    }
//};

DECLARE_MESSAGE(LoginReq);

namespace xx {
    struct Test {
    };
}

DECLARE_MESSAGE_BY_NAME(xx::Test, "xx-Test");


struct LoginRep {

    static std::string msgName() { return "rep"; }

    std::string info = "hello";

    std::vector<uint32_t> values = {1, 2, 3, 4, 5, 6};

    std::string serialize() const {
        ProtoArchiver ar;
        ar << info;
        ar << values;
        return ar.SerializeAsString();
    }

    bool deserialize(const std::string &data) {
        ProtoArchiver ar;
        if (ar.ParseFromString(data)) {
            ar >> info;
            ar >> values;
            return true;
        }
        return false;
    }

    void dump() const {
        std::cout << "LoginRep:" << info << " - ";
        for (auto &v : values)
            std::cout << v << ",";
        std::cout << std::endl;
    }
};

void test_2() {
    std::cout << "--------" << __PRETTY_FUNCTION__ << std::endl;

    LoginReq send;

    MessageBuffer msgbuf;
    msgbuf.writeByName(send);

    hexdump(msgbuf.str());

    MessageDispatcher<> dispatcher;
    dispatcher
            .on<LoginReq, LoginRep>([](const LoginReq &request) {
                std::cout << "----" << __PRETTY_FUNCTION__ << std::endl;
                request.dump();
                LoginRep reply;
                reply.info = "hello !!";
                return reply;
            })
            .on<LoginRep>([](const LoginRep &msg) {
                std::cout << "----" << __PRETTY_FUNCTION__ << std::endl;
                msg.dump();
            });

    MessageBufferPtr reply = dispatcher.dispatch(msgbuf.str());
    if (reply) {
        if (!dispatcher.dispatch(reply->str())) {
            std::cout << "---- over" << std::endl;
        }
    }

}

int main(int argc, const char *argv[]) {
    test_1();
    test_2();
//    test_byname();
//    test_byid1();
//    test_byid2();
    std::cout << MessageName<xx::Test>::value() << std::endl;
}