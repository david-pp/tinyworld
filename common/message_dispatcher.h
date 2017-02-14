#ifndef TINYWORLD_MESSAGE_DISPATCHER_H
#define TINYWORLD_MESSAGE_DISPATCHER_H

#include <map>
#include <unordered_map>
#include <tuple>
#include <iostream>
#include <memory>
#include <functional>
#include <google/protobuf/message.h>
#include "message.h"

///////////////////////////////////////////////////////////
//
//  基于Protobuf的消息处理和分发(2017/02/04 by David)
//
//  提供了3种模式的应用：
//   1. ProtobufMsgDispatcherByName  - 基于协议名称的
//   2. ProtobufMsgDispatcherByID1   - 基于协议的消息编号(TYPE)
//   3. ProtobufMsgDispatcherByID2   - 基于协议的主、次消息编号(TYPE1、TYPE2)
//
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
//
// 消息处理Handler基类
//
///////////////////////////////////////////////////////////
class ProtobufMsgHandlerBase
{
public:
    typedef google::protobuf::Message Message;
    typedef std::shared_ptr<Message>  MessagePtr;

public:
    ProtobufMsgHandlerBase(uint16 msgtype = 0, const std::string& msgname = "")
            : msgtype_(msgtype), msgname_(msgname) {}

    virtual ~ProtobufMsgHandlerBase() {}

    // prototype
    virtual Message* newMessage() = 0;

    uint16 msgtype() { return msgtype_; }
    uint16 type1() { return MSG_TYPE_1(msgtype_); }
    uint16 type2() { return MSG_TYPE_2(msgtype_); }

    std::string& msgname() { return msgname_; }

    void onBindFailed(const char* log)
    {
        std::cerr << "[FATAL] " << log
                  << ": " << msgname_
                  << " -> "<< msgtype()
                  <<"(" << type1()
                  << "," << type2()
                  << ")" << std::endl;
    }

    //
    // 消息打包:名字为键值
    //
    template <typename MSG>
    static bool packByName(std::string& buf, const MSG& msg)
    {
        const size_t msg_namelen  = msg.descriptor()->full_name().length();
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_namelen + msg_protosize);

        MessageHeader* header = (MessageHeader*)buf.data();
        {
            header->size = msg_namelen + msg_protosize;
            header->type_is_name = 1;
            header->type_len = msg_namelen;
        }

        uint8* msg_name = (uint8*)(buf.data()) + sizeof(MessageHeader);
        memcpy(msg_name, msg.descriptor()->full_name().c_str(), msg_namelen);

        uint8* msg_proto = msg_name + msg_namelen;
        return msg.SerializeToArray(msg_proto, msg_protosize);
    }

    //
    // 消息打包:一个消息编号
    //
    template <typename MSG>
    static bool packByID1(std::string& buf, const MSG& msg)
    {
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_protosize);

        MessageHeader* header = (MessageHeader*)buf.data();
        {
            header->size = msg_protosize;
            header->type_is_name = 0;
            header->type = MSG::TYPE;
        }

        uint8* msg_proto = (uint8*)(buf.data()) + sizeof(MessageHeader);
        return msg.SerializeToArray(msg_proto, msg_protosize);
    }

    //
    // 消息打包:两个消息编号
    //
    template <typename MSG>
    static bool packByID2(std::string& buf, const MSG& msg)
    {
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_protosize);

        MessageHeader* header = (MessageHeader*)buf.data();
        {
            header->size = msg_protosize;
            header->type_is_name = 0;
            header->type = MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2);
        }

        uint8* msg_proto = (uint8*)(buf.data()) + sizeof(MessageHeader);
        return msg.SerializePartialToArray(msg_proto, msg_protosize);
    }

private:
    /// 消息编号
    uint16 msgtype_;
    /// 消息名字
    std::string msgname_;
};

///////////////////////////////////////////////////////////
//
// 支持多参数分发的基类
//
///////////////////////////////////////////////////////////
template<typename... ArgTypes>
struct ProtobufMsgHandler : public ProtobufMsgHandlerBase
{
    using ProtobufMsgHandlerBase::ProtobufMsgHandlerBase;

    virtual void process(MessagePtr msg, ArgTypes... args) const = 0;
};

///////////////////////////////////////////////////////////
//
// 消息处理形如：void handler(const MSG& message, ArgTypes...)
//
///////////////////////////////////////////////////////////
template <typename MSG, typename... ArgTypes>
class ProtobufMsgHandlerT_N : public ProtobufMsgHandler<ArgTypes...>
{
public:
    typedef std::function<void(const MSG& msg, ArgTypes... args)> Handler;

    ProtobufMsgHandlerT_N(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), handler_(handler) {}

    ProtobufMsgHandlerBase::Message* newMessage() final { return new MSG; }

    void process(ProtobufMsgHandlerBase::MessagePtr msg,  ArgTypes... args) const final
    {
        handler_(*(MSG*)msg.get(), args...);
    }

private:
    Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息处理形如：void handler(void)
//
///////////////////////////////////////////////////////////
template <typename MSG, typename... ArgTypes>
class ProtobufMsgHandlerT_Void : public ProtobufMsgHandler<ArgTypes...>
{
public:
    typedef std::function<void(void)> Handler;

    ProtobufMsgHandlerT_Void(uint16 msgtype,  const std::string& msgname, const Handler& handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), handler_(handler) {}

    ProtobufMsgHandlerBase::Message* newMessage() final { return new MSG; }

    void process(ProtobufMsgHandlerBase::MessagePtr msg,  ArgTypes... args) const final
    {
        handler_();
    }

private:
    Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息分发器-使用协议的名字为键值
//
///////////////////////////////////////////////////////////
template <typename... ArgTypes>
class ProtobufMsgDispatcherByName
{
public:
    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;

    //
    // 全局单件
    //
    static ProtobufMsgDispatcherByName<ArgTypes...>& instance()
    {
        static ProtobufMsgDispatcherByName<ArgTypes...> instance_;
        return instance_;
    }

    //
    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
    //
    template <typename MSG>
    ProtobufMsgDispatcherByName<ArgTypes...>& on(const typename ProtobufMsgHandlerT_N<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<MSG, ArgTypes...>(0, MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    template <typename MSG>
    ProtobufMsgDispatcherByName<ArgTypes...>& on_void(const typename ProtobufMsgHandlerT_Void<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<MSG, ArgTypes...>(0, MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // 消息分发
    //
    bool dispatch(const MessageHeader* msgheader,  ArgTypes... args)
    {
        if (0 == msgheader->type_is_name || 0 == msgheader->type_len)
            return false;

        const char* msg_name =(char*)msgheader + sizeof(MessageHeader);
        uint8* msg_proto = (uint8*)msg_name + msgheader->type_len;

        std::string msgname(msg_name, msgheader->type_len);

        auto it = handlers_.find(msgname);
        if (it != handlers_.end())
        {
            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size - msgheader->type_len))
            {
                it->second->process(proto, args...);
                return true;
            }
        }
        else
        {
            std::cout << "ERROR:" << msgname << std::endl;
        }

        return false;
    }

protected:
    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
    {
        if (!handler) return false;

        if (handlers_.find(handler->msgname()) != handlers_.end())
        {
            handler->onBindFailed(__PRETTY_FUNCTION__);
            return false;
        }

        handlers_.insert(std::make_pair(handler->msgname(), handler));
        return true;
    }

private:
    typedef std::unordered_map<std::string, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;
    ProtobufMsgHandlerMap handlers_;
};

///////////////////////////////////////////////////////////
//
// 消息分发器-使用协议的ID为键值)
//
// 1.ProtobufMsgDispatcherByID1
//   Protobuf消息的约定：
//    - MSG::TYPE为消息编号
//
// 2.ProtobufMsgDispatcherByID2
//   Protobuf消息的约定：
//    - MSG::TYPE1 为主消息编号
//    - MSG::TYPE2 为次消息编号
//
///////////////////////////////////////////////////////////
template <typename... ArgTypes>
class ProtobufMsgDispatcherByID1
{
public:
    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;

    //
    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
    //
    template <typename MSG>
    ProtobufMsgDispatcherByID1<ArgTypes...>& on(const typename ProtobufMsgHandlerT_N<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<MSG, ArgTypes...>(MSG::TYPE, MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    template <typename MSG>
    ProtobufMsgDispatcherByID1<ArgTypes...>& on_void(const typename ProtobufMsgHandlerT_Void<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<MSG, ArgTypes...>(MSG::TYPE, MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // 消息分发
    //
    bool dispatch(const MessageHeader* msgheader,  ArgTypes... args)
    {
        if (msgheader->type_is_name > 0)
            return false;

        uint8* msg_proto = (uint8*)msgheader + sizeof(MessageHeader);

        auto it = handlers_.find(msgheader->type);
        if (it != handlers_.end())
        {
            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size))
            {
                it->second->process(proto, args...);
                return true;
            }
        }

        return false;
    }

protected:
    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
    {
        if (!handler) return false;

        if (handlers_.find(handler->msgtype()) != handlers_.end()) {
            handler->onBindFailed(__PRETTY_FUNCTION__);
            return false;
        }

        handlers_.insert(std::make_pair(handler->msgtype(), handler));
        return true;
    }

private:
    typedef std::unordered_map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;

    ProtobufMsgHandlerMap handlers_;
};


template <typename... ArgTypes>
class ProtobufMsgDispatcherByID2
{
public:
    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;

    //
    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
    //
    template <typename MSG>
    ProtobufMsgDispatcherByID2<ArgTypes...>& on(const typename ProtobufMsgHandlerT_N<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<MSG, ArgTypes...>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    template <typename MSG>
    ProtobufMsgDispatcherByID2<ArgTypes...>& on_void(const typename ProtobufMsgHandlerT_Void<MSG, ArgTypes...>::Handler& handler)
    {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<MSG, ArgTypes...>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), MSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // 消息分发
    //
    bool dispatch(const MessageHeader* msgheader,  ArgTypes... args)
    {
        if (msgheader->type_is_name > 0)
            return false;

        uint8* msg_proto = (uint8*)msgheader + sizeof(MessageHeader);

        auto it = handlers_.find(msgheader->type);
        if (it != handlers_.end())
        {
            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size))
            {
                it->second->process(proto, args...);
                return true;
            }
        }

        return false;
    }

protected:
    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
    {
        if (!handler) return false;

        if (handlers_.find(handler->msgtype()) != handlers_.end()) {
            handler->onBindFailed(__PRETTY_FUNCTION__);
            return false;
        }

        handlers_.insert(std::make_pair(handler->msgtype(), handler));
        return true;
    }

private:
    typedef std::unordered_map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;

    ProtobufMsgHandlerMap handlers_;
};


#endif //TINYWORLD_MESSAGE_DISPATCHER_H
