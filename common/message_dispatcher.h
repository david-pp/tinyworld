#ifndef TINYWORLD_MESSAGE_DISPATCHER_H
#define TINYWORLD_MESSAGE_DISPATCHER_H

#include <map>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <functional>
#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>
#include "message.h"

///////////////////////////////////////////////////////////
//
//  基于Protobuf的消息处理和分发(2017/02/04 by David)
//
//  提供了3种模式的应用：
//   1. ProtobufMsgDispatcherByName  - 基于协议名称的
//   2. ProtobufMsgDispatcherByOneID - 基于协议的消息编号(TYPE)
//   3. ProtobufMsgDispatcherByTwoID - 基于协议的主、次消息编号(TYPE1、TYPE2)
//
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
//
// 消息处理Handler
//
///////////////////////////////////////////////////////////

//
// 消息处理基类
//
class ProtobufMsgHandler : boost::noncopyable
{
public:
    struct Args
    {
    };

    template <typename T1>
    struct ArgsT_1 : public Args
    {
        ArgsT_1(const T1& a1) : arg1(a1) {}
        T1 arg1;
    };

    template <typename T1, typename T2>
    struct ArgsT_2 : public Args
    {
        ArgsT_2(const T1& a1, const T2& a2) : arg1(a1), arg2(a2) {}
        T1 arg1;
        T2 arg2;
    };

    typedef google::protobuf::Message Message;
    typedef std::shared_ptr<Message> MessagePtr;
    typedef std::shared_ptr<Args> ArgsPtr;

public:
    ProtobufMsgHandler(uint16 msgtype = 0, const std::string& msgname = "")
            : msgtype_(msgtype), msgname_(msgname) {}

    virtual ~ProtobufMsgHandler() {}
    virtual void process(MessagePtr msg, ArgsPtr args) const = 0;

    // prototype
    virtual Message* newMessage() { return NULL; }

    uint16 msgtype() { return msgtype_; }
    uint16 type1() { return MSG_TYPE_1(msgtype_); }
    uint16 type2() { return MSG_TYPE_2(msgtype_); }

    std::string& msgname() { return msgname_; }

private:
    /// 消息编号
    uint16 msgtype_;
    /// 消息名字
    std::string msgname_;
};

typedef std::shared_ptr<ProtobufMsgHandler> ProtobufMsgHandlerPtr;


//
// 消息处理形如：void handler(void)
//
template <typename MSG>
class ProtobufMsgHandlerT : public ProtobufMsgHandler
{
public:
    typedef std::function<void(void)> Handler;

    ProtobufMsgHandlerT(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler(msgtype, msgname), handler_(handler) {}

    virtual Message* newMessage() { return new MSG; }

    virtual void process(MessagePtr msg, ArgsPtr args) const
    {
        handler_();
    }

private:
    Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message)
//
template <typename MSG>
class ProtobufMsgHandlerT_0 : public ProtobufMsgHandler
{
public:
    typedef std::function<void(MSG*)> Handler;

    ProtobufMsgHandlerT_0(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler(msgtype, msgname), handler_(handler) {}

    virtual Message* newMessage() { return new MSG; }

    virtual void process(MessagePtr msg, ArgsPtr args) const
    {
        handler_((MSG*)msg.get());
    }

private:
    Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message, T1 arg1)
//
template <typename MSG, typename T1>
class ProtobufMsgHandlerT_1 : public ProtobufMsgHandler
{
public:
    typedef std::function<void(MSG*, T1)> Handler;

    typedef ProtobufMsgHandler::ArgsT_1<T1> ArgsType;

    ProtobufMsgHandlerT_1(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler(msgtype, msgname), handler_(handler) {}

    virtual Message* newMessage() { return new MSG; }

    virtual void process(MessagePtr msg, ArgsPtr args) const
    {
        if (args)
        {
            ArgsType* a = (ArgsType*)args.get();
            handler_((MSG*)msg.get(),  a->arg1);
        }
        else
            handler_((MSG*)msg.get(), T1());
    }

private:
    Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message, T1 arg1, T2 arg2)
//
template <typename MSG, typename T1, typename T2>
class ProtobufMsgHandlerT_2 : public ProtobufMsgHandler
{
public:
    typedef std::function<void(MSG*, T1, T2)> Handler;

    typedef ProtobufMsgHandler::ArgsT_2<T1, T2> ArgsType;

    ProtobufMsgHandlerT_2(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler(msgtype, msgname), handler_(handler) {}

    virtual Message* newMessage() { return new MSG; }

    virtual void process(MessagePtr msg, ArgsPtr args) const
    {
        if (args)
        {
            ArgsType* a = (ArgsType*)args.get();
            handler_((MSG*)msg.get(),  a->arg1, a->arg2);
        }
        else
            handler_((MSG*)msg.get(), T1(), T2());
    }

private:
    Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息分发器基类
//
///////////////////////////////////////////////////////////
class ProtobufMsgDispatcherBase
{
public:
    //
    // 消息分发
    //
    bool dispatch(const MessageHeader* msgheader)
    {
        return this->dispatch_(msgheader, ProtobufMsgHandler::ArgsPtr());
    }

    template <typename T1>
    bool dispatch(const MessageHeader* msgheader, T1 arg1)
    {
        typedef typename ProtobufMsgHandler::ArgsT_1<T1> ArgsType;
        ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1));
        return this->dispatch_(msgheader, args);
    }

    template <typename T1, typename T2>
    bool dispatch(const MessageHeader* msgheader, T1 arg1, T2 arg2)
    {
        typedef typename ProtobufMsgHandler::ArgsT_2<T1, T2> ArgsType;
        ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1, arg2));
        return this->dispatch_(msgheader, args);
    }

protected:
    virtual bool dispatch_(const MessageHeader* msgheader, ProtobufMsgHandler::ArgsPtr args) = 0;
};


///////////////////////////////////////////////////////////
//
// 消息分发器-使用协议的名字为键值
//
///////////////////////////////////////////////////////////
class ProtobufMsgDispatcherByName : public  ProtobufMsgDispatcherBase
{
public:
    //
    // 绑定消息处理函数，可以是普通的函数、函数对象、成员函数等可执行体
    //
    //  - bind0<MSG>(handler)           handler没有任何参数
    //  - bind<MSG>(handler)            handler没有额参数
    //  - bind<MSG, T1>(handler)        handler有1个额外参数
    //  - bind<MSG, T1, T2>(handler)    handler有2个额外参数
    //
    template <typename MSG>
    bool bind0(const typename ProtobufMsgHandlerT<MSG>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT<MSG>(0, MSG::descriptor()->full_name(), handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": (" << h->msgname()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG>
    bool bind(const typename ProtobufMsgHandlerT_0<MSG>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_0<MSG>(0, MSG::descriptor()->full_name(), handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": (" << h->msgname()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG, typename T1>
    bool bind(const typename ProtobufMsgHandlerT_1<MSG, T1>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_1<MSG, T1>(0, MSG::descriptor()->full_name(), handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": (" << h->msgname()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG, typename T1, typename T2>
    bool bind(const typename ProtobufMsgHandlerT_2<MSG, T1, T2>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_2<MSG, T1, T2>(0, MSG::descriptor()->full_name(), handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": (" << h->msgname()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
    {
        if (!handler) return false;

        if (handlers_.find(handler->msgname()) != handlers_.end())
            return false;

        handlers_.insert(std::make_pair(handler->msgname(), handler));
        return true;
    }


    //
    // 消息打包
    //
    template <typename MSG>
    static bool pack(std::string& buf, MSG* msg)
    {
        const size_t msg_namelen  = msg->descriptor()->full_name().length();
        const size_t msg_protosize = msg->ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_namelen + msg_protosize);

        MessageHeader* header = (MessageHeader*)buf.data();
        {
            header->size = msg_namelen + msg_protosize;
            header->type_is_name = 1;
            header->type_len = msg_namelen;
        }

        uint8* msg_name = (uint8*)(buf.data()) + sizeof(MessageHeader);
        memcpy(msg_name, msg->descriptor()->full_name().c_str(), msg_namelen);

        uint8* msg_proto = msg_name + msg_namelen;
        return msg->SerializePartialToArray(msg_proto, msg_protosize);
    }

protected:
    bool dispatch_(const MessageHeader* msgheader, ProtobufMsgHandler::ArgsPtr args) final
    {
        if (0 == msgheader->type_is_name || 0 == msgheader->type_len)
            return false;

        const char* msg_name =(char*)msgheader + sizeof(MessageHeader);
        uint8* msg_proto = (uint8*)msg_name + msgheader->type_len;

        std::string msgname(msg_name, msgheader->type_len);

        ProtobufMsgHandlerMap::iterator it = handlers_.find(msgname);
        if (it != handlers_.end())
        {
            ProtobufMsgHandler::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size - msgheader->type_len))
            {
                it->second->process(proto, args);
                return true;
            }
        }

        return false;
    }

private:
    typedef std::unordered_map<std::string, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;

    ProtobufMsgHandlerMap handlers_;
};


///////////////////////////////////////////////////////////
//
// 消息分发器-使用协议的ID为键值(支持两种Policy)
//
// 1.ProtobufMsgDispatcherByOneID
//   Protobuf消息的约定：
//    - MSG::TYPE为消息编号
//
// 2.ProtobufMsgDispatcherByTwoID
//   Protobuf消息的约定：
//    - MSG::TYPE1 为主消息编号
//    - MSG::TYPE2 为次消息编号
//
///////////////////////////////////////////////////////////

template <typename MSG>
struct MsgTypePolicy_1
{
    static const uint16 msgtype = MSG::TYPE;
};

template <typename MSG>
struct MsgTypePolicy_2
{
    static const uint16 msgtype = MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2);
};

template <template <typename T> class MsgTypePolicy = MsgTypePolicy_1>
class ProtobufMsgDispatcherByID : public ProtobufMsgDispatcherBase
{
public:
    //
    // 绑定消息处理函数，可以是普通的函数、函数对象、成员函数等可执行体
    //
    //  - bind0<MSG>(handler)           handler没有任何参数
    //  - bind<MSG>(handler)            handler没有额参数
    //  - bind<MSG, T1>(handler)        handler有1个额外参数
    //  - bind<MSG, T1, T2>(handler)    handler有2个额外参数
    //
    template <typename MSG>
    bool bind0(const typename ProtobufMsgHandlerT<MSG>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT<MSG>(MsgTypePolicy<MSG>::msgtype, "", handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": " << h->msgtype()
                      <<"(" << h->type1()
                      << "," << h->type2()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG>
    bool bind(const typename ProtobufMsgHandlerT_0<MSG>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_0<MSG>(MsgTypePolicy<MSG>::msgtype, "", handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": " << h->msgtype()
                      <<"(" << h->type1()
                      << "," << h->type2()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG, typename T1>
    bool bind(const typename ProtobufMsgHandlerT_1<MSG, T1>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_1<MSG, T1>(MsgTypePolicy<MSG>::msgtype, "", handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": " << h->msgtype()
                      <<"(" << h->type1()
                      << "," << h->type2()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MSG, typename T1, typename T2>
    bool bind(const typename ProtobufMsgHandlerT_2<MSG, T1, T2>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_2<MSG, T1, T2>(MsgTypePolicy<MSG>::msgtype, "", handler));
        if (!bindHandlerPtr(h))
        {
            std::cerr << "[FATAL] " << __PRETTY_FUNCTION__
                      << ": " << h->msgtype()
                      <<"(" << h->type1()
                      << "," << h->type2()
                      << ")" << std::endl;
            return false;
        }
        return true;
    }

    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
    {
        if (!handler) return false;

        if (handlers_.find(handler->msgtype()) != handlers_.end())
            return false;

        handlers_.insert(std::make_pair(handler->msgtype(), handler));
        return true;
    }

    //
    // 消息打包
    //
    template <typename MSG>
    static bool pack(std::string& buf, MSG* msg)
    {
        const size_t msg_protosize = msg->ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_protosize);

        MessageHeader* header = (MessageHeader*)buf.data();
        {
            header->size = msg_protosize;
            header->type_is_name = 0;
            header->type = MsgTypePolicy<MSG>::msgtype;
        }

        uint8* msg_proto = (uint8*)(buf.data()) + sizeof(MessageHeader);
        return msg->SerializePartialToArray(msg_proto, msg_protosize);
    }

protected:
    //
    // 消息分发
    //
    bool dispatch_(const MessageHeader* msgheader, ProtobufMsgHandler::ArgsPtr args) final
    {
        if (msgheader->type_is_name > 0)
            return false;

        uint8* msg_proto = (uint8*)msgheader + sizeof(MessageHeader);

        ProtobufMsgHandlerMap::iterator it = handlers_.find(msgheader->type);
        if (it != handlers_.end())
        {
            ProtobufMsgHandler::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size))
            {
                it->second->process(proto, args);
                return true;
            }
        }

        return false;
    }

private:
    typedef std::unordered_map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;

    ProtobufMsgHandlerMap handlers_;
};

typedef ProtobufMsgDispatcherByID<MsgTypePolicy_1> ProtobufMsgDispatcherByOneID;
typedef ProtobufMsgDispatcherByID<MsgTypePolicy_2> ProtobufMsgDispatcherByTwoID;


#endif //TINYWORLD_MESSAGE_DISPATCHER_H
