#include <iostream>
#include "message_dispatcher.h"
#include "tinyserializer_proto.h"

#include "test_msg.pb.h"

//
// 模拟客户端
//
template<typename MsgDispatcherT>
struct Client {

    // 模拟发消息
    template<typename MsgT>
    std::string sendCmd(const MsgT &msg) {
        MessageBuffer buffer;
        MsgDispatcherT::template write2Buffer(buffer, msg);
        return buffer.str();
    }

    MsgDispatcherT dispatcher;
};

//
// 模拟服务端
//
template<typename MsgDispatcherT>
struct Server {

    MsgDispatcherT dispatcher;
};

//
// 演示: 使用Proto做协议
//
//  方式1 - Proto的名字做类型(推荐)
//  方式2 - 自定义类型编码(uint16)
//  方式3 - 自定义主次类型比那么(uint8, uint8)
//
//          ---- Cmd::LoginRequest -->
// Client-{                           }-Server
//          <--- Cmd::LoginReply  ----
//
void test_proto_by_name() {
    std::cout << "\n--------" << __PRETTY_FUNCTION__ << std::endl;

    // Client: s初始化
    Client<MessageNameDispatcher<>> client;
    client.dispatcher
            .on<Cmd::LoginReply>([](const Cmd::LoginReply &msg) {
                std::cout << "Client-\t" << msg.ShortDebugString() << std::endl;
            });

    // Client: 创建消息、发送消息
    Cmd::LoginRequest cmd;
    cmd.set_id(12345);
    cmd.set_password("passwd");
    cmd.set_type(20);
    std::string data = client.sendCmd(cmd);
    std::cout << "Client-\tsendCmd: " << cmd.ShortDebugString() << std::endl;

    // Server: 初始化
    Server<MessageNameDispatcher<>> server;
    server.dispatcher
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &request) {
                std::cout << "Server-\t" << request.ShortDebugString() << std::endl;
                Cmd::LoginReply reply;
                reply.set_info("hello");
                return reply;
            });

    // Server: 收到消息进行分发
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    if (reply) {
        // 有返回则返回给客户端，最终执行Client的消息分发
        client.dispatcher.dispatch(reply->str());
    }
}

//
// 自定义编号
//
DECLARE_MESSAGE_BY_TYPE(Cmd::LoginRequest, 1);

DECLARE_MESSAGE_BY_TYPE(Cmd::LoginReply, 2);

//DECLARE_MESSAGE_BY_TYPE2(Cmd::LoginRequest, 1, 1);
//DECLARE_MESSAGE_BY_TYPE2(Cmd::LoginReply, 1, 2);

void test_proto_by_type() {
    std::cout << "\n--------" << __PRETTY_FUNCTION__ << std::endl;

    // Client: 初始化
    Client<MessageDispatcher<>> client;
    client.dispatcher
            .on<Cmd::LoginReply>([](const Cmd::LoginReply &msg) {
                std::cout << "Client-\t" << msg.ShortDebugString() << std::endl;
            });

    // Client: 创建消息、发送消息
    Cmd::LoginRequest cmd;
    cmd.set_id(12345);
    cmd.set_password("passwd");
    cmd.set_type(20);
    std::string data = client.sendCmd(cmd);
    std::cout << "Client-\tsendCmd: " << cmd.ShortDebugString() << std::endl;


    // Server: 初始化
    Server<MessageDispatcher<>> server;
    server.dispatcher
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &request) {
                std::cout << "Server-\t" << request.ShortDebugString() << std::endl;
                Cmd::LoginReply reply;
                reply.set_info("hello");
                return reply;
            });

    // Server: 收到消息进行分发
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    if (reply) {
        // 有返回则返回给客户端，最终执行Client的消息分发
        client.dispatcher.dispatch(reply->str());
    }
}


//
// 演示: 使用用户自定义类型做协议
//
//  方式1 - 自定义名字做类型
//  方式2 - 自定义类型编码(uint16)
//  方式3 - 自定义主次类型比那么(uint8, uint8)
//
//          ---- LoginReq -->
// Client-{                  }-Server
//          <--- LoginRep ---
//

// 自定义类型: Request
struct LoginReq {
    int type = 0;
    std::string password = "";

    // 侵入式实现协议名/编码的约定(一般情况仅需要一种即可)
    static std::string msgName() { return "request"; }

    static uint16_t msgType() { return 1; }

    // 侵入式实现ProtoSerializer的约定
    std::string serialize() const {
        Cmd::LoginRequest proto;
        proto.set_id(12345);
        proto.set_password(password);
        proto.set_type(type);
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

// 自定义类型: Reply
struct LoginRep {
    std::string info;
    std::vector<uint32_t> values;

    void init() {
        info = "hello";
        values = {1, 2, 3, 4, 5, 6};
    }

    void dump() const {
        std::cout << "LoginRep:" << info << " - ";
        for (auto &v : values)
            std::cout << v << ",";
        std::cout << std::endl;
    }
};

//
// 非侵入式实现ProtoSerializer的约定
//
template<>
struct ProtoSerializer<LoginRep> {
    std::string serialize(const LoginRep &msg) const {
        ProtoArchiver ar;
        ar << msg.info;
        ar << msg.values;
        return ar.SerializeAsString();
    }

    bool deserialize(LoginRep &msg, const std::string &data) const {
        ProtoArchiver ar;
        if (ar.ParseFromString(data)) {
            ar >> msg.info;
            ar >> msg.values;
            return true;
        }
        return false;
    }
};

//
// 非侵入式指定对应的协议名/编号
//
DECLARE_MESSAGE(LoginRep);
//DECLARE_MESSAGE_BY_NAME(LoginReq, "reply");

//template <>
//struct MessageName<LoginReq> {
//    static std::string value() {
//        return "req";
//    }
//};

DECLARE_MESSAGE_BY_TYPE(LoginRep, 2);
//DECLARE_MESSAGE_BY_TYPE2(LoginRep, 1, 2);


void test_userdefine_by_name() {
    std::cout << "\n--------" << __PRETTY_FUNCTION__ << std::endl;

    // Client: 初始化
    Client<MessageNameDispatcher<>> client;
    client.dispatcher
            .on<LoginRep>([](const LoginRep &msg) {
                std::cout << "Client-\t";
                msg.dump();
            });

    // Client: 创建消息、发送消息
    LoginReq cmd;
    cmd.type = 1024;
    cmd.password = "123456";
    std::string data = client.sendCmd(cmd);
    std::cout << "Client-\tsendCmd: ";
    cmd.dump();

    // Server: 初始化
    Server<MessageNameDispatcher<>> server;
    server.dispatcher
            .on<LoginReq, LoginRep>([](const LoginReq &request) {
                std::cout << "Server-\t";
                request.dump();

                LoginRep reply;
                reply.init();
                return reply;
            });

    // Server: 收到消息进行分发
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    if (reply) {
        // 有返回则返回给客户端，最终执行Client的消息分发
        client.dispatcher.dispatch(reply->str());
    }
}

void test_userdefine_by_type() {
    std::cout << "\n--------" << __PRETTY_FUNCTION__ << std::endl;

    // Client: 初始化
    Client<MessageDispatcher<>> client;
    client.dispatcher
            .on<LoginRep>([](const LoginRep &msg) {
                std::cout << "Client-\t";
                msg.dump();
            });


    // Client: 创建消息、发送消息
    LoginReq cmd;
    cmd.type = 1024;
    cmd.password = "123456";
    std::string data = client.sendCmd(cmd);
    std::cout << "Client-\tsendCmd: ";
    cmd.dump();

    // 服务端初始化
    Server<MessageDispatcher<>> server;
    server.dispatcher
            .on<LoginReq, LoginRep>([](const LoginReq &request) {
                std::cout << "Server-\t";
                request.dump();

                LoginRep reply;
                reply.init();
                return reply;
            });

    // Server: 收到消息进行分发
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    if (reply) {
        // 有返回则返回给客户端，最终执行Client的消息分发
        client.dispatcher.dispatch(reply->str());
    }
}

//
// 演示：使用bind expression、dispatch传入其他参数
//

// 玩家对象
struct Player {
    int v = 314;
};

// 登录模块
struct LoginModule {
    static LoginModule &instance() {
        static LoginModule login;
        return login;
    }

    void doLogin(const Cmd::LoginRequest &msg, Player *player) {
        std::cout << "-- " << __PRETTY_FUNCTION__ << std::endl;
        std::cout << msg.DebugString() << std::endl;
        std::cout << "v: " << player->v << std::endl;
    }
};

void test_expansion() {
    std::cout << "\n--------" << __PRETTY_FUNCTION__ << std::endl;

    // Client: 初始化
    Client<MessageNameDispatcher<>> client;

    // Client: 创建消息、发送消息
    Cmd::LoginRequest cmd;
    cmd.set_id(12345);
    cmd.set_password("passwd");
    cmd.set_type(20);
    std::string data = client.sendCmd(cmd);
    std::cout << "Client-\tsendCmd: " << cmd.ShortDebugString() << std::endl;


    // Server: 初始化
    Server<MessageNameDispatcher<Player *>> server;
    server.dispatcher
            .on<Cmd::LoginRequest>(
                    std::bind(&LoginModule::doLogin,
                              LoginModule::instance(),
                              std::placeholders::_1, std::placeholders::_2));

    // Server: 收到消息进行分发(需要指定一个角色指针)
    Player *player = new Player;
    MessageBufferPtr reply = server.dispatcher.dispatch(data, player);
    if (reply) {
        // 有返回则返回给客户端，最终执行Client的消息分发
        client.dispatcher.dispatch(reply->str());
    }
}

int main(int argc, const char *argv[]) {
    test_proto_by_name();
    test_proto_by_type();
    test_userdefine_by_name();
    test_userdefine_by_type();
    test_expansion();
}