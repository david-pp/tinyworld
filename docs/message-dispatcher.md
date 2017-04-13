Message Dispatcher的设计和用法
============================

## 1.设计

核心概念：

- 流化(MessageBuffer)
    - 消息结构体可以是Protobuf生成的结构体，也可以是用户自定义的类型，用户自定义类型需要遵循序列化和分发器的约定。
    - 完成消息结构体的序列化、压缩、加密等。（当前仅实现了序列化）。
   
- 消息分发(MessageDispatcher)
  - 可以使用三种方式进行分发：类型名、类型唯一编码、主次类型编码。
  - 分发的时候除了指定消息结构体，还可以传入其他参数，可以传入的参数数量不限。
  - 回调使用`std::function`，可以传入各种可执行体（普通函数、成员函数、函数对象、Lambda表达式、Bind表达式等）。


设计考量：

- Reactive Programing，简便易懂
- 快速失败：不遵循序列化约束和相应分发器约束的，直接编译不过；不小心类型重复的，在注册回调的时候抛出异常或输出错误（当前采用的是输出到`std::cerr`）。

## 2.基本用法

通信模型：

```
 +--------+                              +--------+
 |        |  ---- Cmd::LoginRequest -->  |        |
 | Client |                              | Server |
 |        |  <--- Cmd::LoginReply  ----  |        |                            
 +--------|                              +--------+
```

应用层操作：
 
- 定义消息
- 创建消息并发送
- 注册该消息的回调

网络层调用：

- 消息分发

### 2.1 分发器的类型

- MessageDispatcher ：使用`uint16_t`的数字作为类型，或者使用主次两个`uint8_t`的数字作为组合类型。约束（非侵入型优先）：
    - 非侵入型
        - 偏特化：
          ```c++
          template <>
          struct MessageTypeCode<MsgT> {
            static uint16_t value() { return xx; }
          }
          ```
        - 使用帮助宏：
           ```c++
           DECLARE_MESSAGE_BY_TYPE(MsgT, 1024)    : MsgT -> 1024
           DECLARE_MESSAGE_BY_TYPE2(MsgT, 20, 30) : MsgT -> (20, 30)
           ```
      
    - 侵入型
        - 静态成员函数：`uint16_t MsgT::msgType()`
     
- MessageNameDispatcher ：使用消息名作为类型。约束（非侵入型优先）：
    - 非侵入型
        - 偏特化：
          ```c++
          template <>
          struct MessageName<MsgT> {
            static std::string value() { return "xx"; }
          }
          ```
        - 使用帮助宏：
           ```c++
           DECLARE_MESSAGE(MsgT)                  : MsgT -> "MsgT"
           DECLARE_MESSAGE_BY_NAME(MsgT, "msg")   : MsgT -> "msg"
           ```
      
    - 侵入型
        - 静态成员函数：`std::string MsgT::msgName()`
     

### 2.2 使用Proto作为消息结构体

- 定义消息

``` proto
package Cmd;

message LoginRequest {
    required uint32 id   = 1;     // 连接ID
    required uint32 type = 2;     // 连接类型
    optional string password = 3; // 验证密码
    optional string name = 4;
}

message LoginReply {
    optional string info = 1;
}
```

- 创建消息并发送

```c++
    Cmd::LoginRequest cmd;
    cmd.set_id(12345);
    cmd.set_password("passwd");
    cmd.set_type(20);
    client.sendCmd(cmd);
```
- 注册该消息的回调

```c++
    server.dispatcher
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &request) {
                std::cout << "Server-\t" << request.ShortDebugString() << std::endl;
                Cmd::LoginReply reply;
                reply.set_info("hello");
                return reply;
            });
```

- 消息分发

```
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    
    // 有返回则返回给客户端
    if (reply) {
        server.sendToClient(reply->str());
    }
```

### 2.3 使用用户定义结构体作为消息

> 注意：处理消息定义方式有所不同，其他操作和上面基本上一样

- 定义消息

``` c++
// 侵入式定义：LoginReq

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
};

////////////////////////////////////////////

// 非侵入式定义：LoginRep

struct LoginRep {
    std::string info;
    std::vector<uint32_t> values;
};

// 非侵入式实现ProtoSerializer的约定
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


// 非侵入式指定对应的协议名/编号(选其一)
DECLARE_MESSAGE(LoginRep);
DECLARE_MESSAGE_BY_NAME(LoginReq, "reply");
DECLARE_MESSAGE_BY_TYPE(LoginRep, 2);
DECLARE_MESSAGE_BY_TYPE2(LoginRep, 1, 2);
```

- 创建消息并发送

```c++
    LoginReq cmd;
    cmd.type = 1024;
    cmd.password = "123456";
    client.sendCmd(cmd);
```
- 注册该消息的回调

```c++
    server.dispatcher
            .on<LoginReq, LoginRep>([](const LoginReq &request) {
                // TODO: ...
                
                LoginRep reply;
                reply.info = "hello";
                reply.values = {1, 2, 3, 4, 5, 6}
                return reply;
            });
```

- 消息分发

```
    MessageBufferPtr reply = server.dispatcher.dispatch(data);
    
    // 有返回则返回给客户端
    if (reply) {
        server.sendToClient(reply->str());
    }
```


## 3.扩展用法

### 3.1 分发支持传入多个参数

- 模拟登录模块

``` c++
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

```

- 分发器的类型

模板参数列表对应着回调函数除了消息的其他参数列表。

```c++
    MessageNameDispatcher<Player *> 
    MessageDispatcher<Player *>
```

- 注册回调

可以理解为：当收到Cmd::LoginRequest的时候，去执行LoginModule::doLogin函数。

```c++
    server.dispatcher
            .on<Cmd::LoginRequest>(
                    std::bind(&LoginModule::doLogin,
                              LoginModule::instance(),
                              std::placeholders::_1, std::placeholders::_2));
```

- 消息分发

```c++
    // 注意，在此多传入了一个Player的指针
    
    server.dispatcher.dispatch(data, player);
```

### 3.2 回调支持成员函数(见上面)


### 4. TODO

- 支持压缩
- 支持加密
- 完善依赖的序列化模块
