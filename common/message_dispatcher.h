// Copyright (c) 2017 david++
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TINYWORLD_MESSAGE_DISPATCHER_H
#define TINYWORLD_MESSAGE_DISPATCHER_H

#include "tinyworld.h"

#include <map>
#include <cstring>
#include <unordered_map>
#include <tuple>
#include <iostream>
#include <memory>
#include <functional>
#include <exception>

#include "message.h"
#include "tinyserializer.h"


class MsgDispatcherError : public std::exception {
public:
    MsgDispatcherError(const std::string &msg)
            : errmsg_(msg) {}

    virtual const char *what() const noexcept {
        return errmsg_.c_str();
    }

private:
    std::string errmsg_;
};

template<typename MsgT, int>
struct MessageNameImpl;

#include <google/protobuf/message.h>

template<typename MsgT>
struct MessageName {
    static std::string value() {
        return MessageNameImpl<MsgT,
                std::is_base_of<google::protobuf::Message, MsgT>::value ? 0 : 1>::value();
    }
};

template<typename MsgT>
struct MessageNameImpl<MsgT, 0> {
    static std::string value() {
        return MsgT::descriptor()->full_name();
    }
};

template<typename MsgT>
struct MessageNameImpl<MsgT, 1> {
    static std::string value() {
        return MsgT::msgName();
    }
};


template<typename MsgT>
struct MessageTypeCode {
    static uint16 value() {
        return MsgT::msgType();
    }
};

#define DECLARE_MESSAGE(MsgType) \
    template <> struct MessageName<MsgType> { \
        static std::string value() { return #MsgType; } }

#define DECLARE_MESSAGE_BY_NAME(MsgType, name) \
    template <> struct MessageName<MsgType> { \
        static std::string value() { return name; } }

#define DECLARE_MESSAGE_BY_TYPE(MsgType, type) \
    template <> struct MessageName<MsgType> { \
        static uint16 value() { return type; } }

#define DECLARE_MESSAGE_BY_TYPE2(MsgType, type1, type2) \
    template <> struct MessageName<MsgType> { \
        static uint16 value() { return MAKE_MSG_TYPE(type1, type2); } }


//
// Message Streaming
//
class MessageBuffer {
public:
    MessageBuffer() {
    }

    MessageBuffer(const std::string &data) {
        buf_ = data;
    }

    ~MessageBuffer() {
    }

    size_t size() { return buf_.size(); }

    const uint8_t *data() { return (uint8_t *) (buf_.data()); }

    std::string &str() { return buf_; }


    template<template<typename T> class SerializerT = ProtoSerializer, typename MsgT>
    bool writeByNameOrID(const std::string &name, uint16 type, const MsgT &msg) {
        if (name.size() > 0)
            return packMsgByName<SerializerT, MsgT>(buf_, name, msg);

        return false;
    };

    template<template<typename T> class SerializerT = ProtoSerializer, typename MsgT>
    bool writeByName(const MsgT &msg) {
        return packMsgByName<SerializerT, MsgT>(buf_, MessageName<MsgT>::value(), msg);
    };

public:
    template<template<typename T> class SerializerT = ProtoSerializer, typename MsgT>
    static bool packMsgByName(std::string &buf, const std::string &name, const MsgT &msg) {
        if (name.empty()) return false;

        std::string data = serialize<SerializerT, MsgT>(msg);
        if (data.empty()) return false;

        buf.resize(sizeof(MessageHeader) + name.size() + data.size());

        MessageHeader *header = (MessageHeader *) buf.data();
        {
            header->size = name.size() + data.size();
            header->type_is_name = 1;
            header->type_len = name.size();
        }

        uint8_t *msg_name = (uint8_t *) (buf.data()) + sizeof(MessageHeader);
        memcpy(msg_name, name.c_str(), name.size());

        uint8_t *msg_proto = msg_name + name.size();
        memcpy(msg_proto, data.data(), data.size());
        return true;
    }

private:
    std::string buf_;
};


using MessageBufferPtr = std::shared_ptr<MessageBuffer>;


template<typename... ArgTypes>
class MessageHandlerBase {
public:
    MessageHandlerBase(uint16 msgtype = 0, const std::string &msgname = "")
            : msgtype_(msgtype), msgname_(msgname) {}

    virtual ~MessageHandlerBase() {}

    //
    // 处理Proto消息
    //
    // 返回值
    //  - NULL   : 无返回值
    //  - BUF指针 : 回调有返回
    //
    virtual MessageBufferPtr process(const std::string &msgbody, ArgTypes... args) const = 0;


    uint16 msgtype() const { return msgtype_; }

    uint16 type1() const { return MSG_TYPE_1(msgtype_); }

    uint16 type2() const { return MSG_TYPE_2(msgtype_); }

    const std::string &msgname() const { return msgname_; }

    void onBindFailed(const char *log) {
        std::cerr << "[FATAL] " << log
                  << ": " << msgname_
                  << " -> " << msgtype()
                  << "(" << type1()
                  << "," << type2()
                  << ")" << std::endl;
    }

private:
    uint16 msgtype_;
    std::string msgname_;
};

template<typename DispatcherT,
        typename RequestT,
        typename ReplyT,
        template<typename> class SerializerT,
        typename... ArgTypes>
class MessageHandlerT_N : public MessageHandlerBase<ArgTypes...> {
public:
    typedef std::function<ReplyT(const RequestT &msg, ArgTypes... args)> Handler;

    MessageHandlerT_N(uint16 msgtype,
                      const std::string &msgname,
                      const Handler &handler)
            : MessageHandlerBase<ArgTypes...>(msgtype, msgname),
              handler_(handler) {}

    virtual ~MessageHandlerT_N() {}

    MessageBufferPtr process(const std::string &msgbody, ArgTypes... args) const final {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        RequestT request;
        if (deserialize<SerializerT, RequestT>(request, msgbody)) {
            ReplyT reply = handler_(request, args...);

            auto buff = std::make_shared<MessageBuffer>();
            if (buff->writeByNameOrID<SerializerT, ReplyT>(MessageName<ReplyT>::value(), 0, reply))
                return buff;
            else
                throw MsgDispatcherError("Reply write2Buffer failed");
        } else {
            throw MsgDispatcherError("deserialize faileds");
            return nullptr;
        }
    }

private:
    Handler handler_;
};

template<typename DispatcherT,
        typename RequestT,
        template<typename> class SerializerT,
        typename... ArgTypes>
class MessageHandlerT_N<DispatcherT, RequestT, void, SerializerT, ArgTypes...>
        : public MessageHandlerBase<ArgTypes...> {
public:
    typedef std::function<void(const RequestT &msg, ArgTypes... args)> Handler;

    MessageHandlerT_N(uint16 msgtype,
                      const std::string &msgname,
                      const Handler &handler)
            : MessageHandlerBase<ArgTypes...>(msgtype, msgname),
              handler_(handler) {}

    virtual ~MessageHandlerT_N() {}

    MessageBufferPtr process(const std::string &msgbody, ArgTypes... args) const final {
        RequestT request;
        if (deserialize<SerializerT, RequestT>(request, msgbody)) {
            handler_(request, args...);
            return nullptr;
        } else {
            throw MsgDispatcherError("deserialize faileds");
            return nullptr;
        }
    }

private:
    Handler handler_;
};


template<typename... ArgTypes>
class MessageDispatcher {
public:
    typedef MessageDispatcher<ArgTypes...> DispatcherType;
    typedef std::shared_ptr<MessageHandlerBase<ArgTypes...> > MessageHandlerPtr;

    template<typename RequestT, typename ReplyT, template<typename T> class SerializerT>
    using MessageHandlerType = MessageHandlerT_N<DispatcherType, RequestT, ReplyT, SerializerT, ArgTypes...>;

    //
    // Default instance
    //
    static DispatcherType &instance() {
        static DispatcherType instance_;
        return instance_;
    }

    //
    // Register message Handler, could be :
    //  - normal function
    //  - function object
    //  - lambda
    //  - bind expression
    //
    template<typename RequestT, typename ReplyT = void, template<typename T> class SerializerT = ProtoSerializer>
    DispatcherType &
    on(const typename MessageHandlerType<RequestT, ReplyT, SerializerT>::Handler &handler) {
        MessageHandlerPtr h(new MessageHandlerType<RequestT, ReplyT, SerializerT>(
                0, MessageName<RequestT>::value(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // Message Dispatching
    //
    MessageBufferPtr dispatch(const std::string &msgdata, ArgTypes... args) {
        MessageHeader *msgheader = (MessageHeader *) msgdata.data();

        if (0 == msgheader->type_is_name || 0 == msgheader->type_len) {
            throw MsgDispatcherError("message header error");
            return nullptr;
        }

        const char *msg_name = (char *) msgheader + sizeof(MessageHeader);
        uint8_t *msg_body = (uint8_t *) msg_name + msgheader->type_len;

        std::string msgname(msg_name, msgheader->type_len);
        std::string msgbody((char *) msg_body, msgheader->size - msgheader->type_len);

        auto it = handlers_.find(msgname);
        if (it != handlers_.end()) {
            return it->second->process(msgbody, args...);
        } else {
            throw MsgDispatcherError("handler not exist for : " + msgname);
        }

        return nullptr;
    }

protected:
    bool bindHandlerPtr(MessageHandlerPtr handler) {
        if (!handler) return false;

        if (handlers_.find(handler->msgname()) != handlers_.end()) {
            handler->onBindFailed(__PRETTY_FUNCTION__);
            return false;
        }

        handlers_.insert(std::make_pair(handler->msgname(), handler));
        return true;
    }

private:
    typedef std::unordered_map<std::string, MessageHandlerPtr> MessageHandlerMap;
    MessageHandlerMap handlers_;
};



// TODO: delete following after refactor
#include <google/protobuf/message.h>
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




// TODO:返回值采用右值引用
// TODO:待ByName完善后再打开ByID1和ByID2



///////////////////////////////////////////////////////////
//
// 消息处理Handler基类
//
///////////////////////////////////////////////////////////
class ProtobufMsgHandlerBase {
public:
    typedef google::protobuf::Message Message;
    typedef std::shared_ptr<Message> MessagePtr;
    typedef std::shared_ptr<std::string> BufPtr;

public:
    ProtobufMsgHandlerBase(uint16 msgtype = 0, const std::string &msgname = "")
            : msgtype_(msgtype), msgname_(msgname) {}

    virtual ~ProtobufMsgHandlerBase() {}

    // prototype
    virtual Message *newMessage() = 0;

    uint16 msgtype() { return msgtype_; }

    uint16 type1() { return MSG_TYPE_1(msgtype_); }

    uint16 type2() { return MSG_TYPE_2(msgtype_); }

    std::string &msgname() { return msgname_; }

    void onBindFailed(const char *log) {
        std::cerr << "[FATAL] " << log
                  << ": " << msgname_
                  << " -> " << msgtype()
                  << "(" << type1()
                  << "," << type2()
                  << ")" << std::endl;
    }

    //
    // 消息打包:名字为键值
    //
    template<typename MSG>
    static bool packByName(std::string &buf, const MSG &msg) {
        const size_t msg_namelen = msg.descriptor()->full_name().length();
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_namelen + msg_protosize);

        MessageHeader *header = (MessageHeader *) buf.data();
        {
            header->size = msg_namelen + msg_protosize;
            header->type_is_name = 1;
            header->type_len = msg_namelen;
        }

        uint8 *msg_name = (uint8 *) (buf.data()) + sizeof(MessageHeader);
        memcpy(msg_name, msg.descriptor()->full_name().c_str(), msg_namelen);

        uint8 *msg_proto = msg_name + msg_namelen;
        return msg.SerializeToArray(msg_proto, msg_protosize);
    }

    //
    // 消息打包:一个消息编号
    //
    template<typename MSG>
    static bool packByID1(std::string &buf, const MSG &msg) {
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_protosize);

        MessageHeader *header = (MessageHeader *) buf.data();
        {
            header->size = msg_protosize;
            header->type_is_name = 0;
            header->type = MSG::TYPE;
        }

        uint8 *msg_proto = (uint8 *) (buf.data()) + sizeof(MessageHeader);
        return msg.SerializeToArray(msg_proto, msg_protosize);
    }

    //
    // 消息打包:两个消息编号
    //
    template<typename MSG>
    static bool packByID2(std::string &buf, const MSG &msg) {
        const size_t msg_protosize = msg.ByteSizeLong();

        buf.resize(sizeof(MessageHeader) + msg_protosize);

        MessageHeader *header = (MessageHeader *) buf.data();
        {
            header->size = msg_protosize;
            header->type_is_name = 0;
            header->type = MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2);
        }

        uint8 *msg_proto = (uint8 *) (buf.data()) + sizeof(MessageHeader);
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
struct ProtobufMsgHandler : public ProtobufMsgHandlerBase {
    using ProtobufMsgHandlerBase::ProtobufMsgHandlerBase;

    //
    // 处理Proto消息
    //
    // 返回值
    //  - NULL   : 无返回值
    //  - BUF指针 : 回调有返回
    //
    virtual BufPtr process(MessagePtr msg, ArgTypes... args) const = 0;
};

///////////////////////////////////////////////////////////
//
// 消息处理形如：ReplyMSG handler(const RequestMSG& message, ArgTypes...)
//
///////////////////////////////////////////////////////////
template<typename Disptacher, typename RequestMSG, typename ReplyMSG, typename... ArgTypes>
class ProtobufMsgHandlerT_N : public ProtobufMsgHandler<ArgTypes...> {
public:
    typedef std::function<ReplyMSG(const RequestMSG &msg, ArgTypes... args)> Handler;

    ProtobufMsgHandlerT_N(Disptacher &dispatcher,
                          uint16 msgtype,
                          const std::string &msgname,
                          const Handler &handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), dispatcher_(dispatcher), handler_(handler) {}

    ProtobufMsgHandlerBase::Message *newMessage() final { return new RequestMSG; }

    ProtobufMsgHandlerBase::BufPtr process(ProtobufMsgHandlerBase::MessagePtr msg, ArgTypes... args) const final {
        ReplyMSG reply = handler_(*(RequestMSG *) msg.get(), args...);

        ProtobufMsgHandlerBase::BufPtr bufptr(new std::string);
        Disptacher::pack(*bufptr.get(), reply);
        return bufptr;
    }

private:
    Disptacher &dispatcher_;
    Handler handler_;
};

template<typename Disptacher, typename RequestMSG, typename... ArgTypes>
class ProtobufMsgHandlerT_N<Disptacher, RequestMSG, void, ArgTypes...> : public ProtobufMsgHandler<ArgTypes...> {
public:
    typedef std::function<void(const RequestMSG &msg, ArgTypes... args)> Handler;

    ProtobufMsgHandlerT_N(Disptacher &dispatcher,
                          uint16 msgtype,
                          const std::string &msgname,
                          const Handler &handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), dispatcher_(dispatcher), handler_(handler) {}

    ProtobufMsgHandlerBase::Message *newMessage() final { return new RequestMSG; }

    ProtobufMsgHandlerBase::BufPtr process(ProtobufMsgHandlerBase::MessagePtr msg, ArgTypes... args) const final {
        handler_(*(RequestMSG *) msg.get(), args...);
        return ProtobufMsgHandlerBase::BufPtr();
    }

private:
    Disptacher &dispatcher_;
    Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息处理形如：ReplyMSG handler(void)
//
///////////////////////////////////////////////////////////
template<typename Disptacher, typename RequestMSG, typename ReplyMSG, typename... ArgTypes>
class ProtobufMsgHandlerT_Void : public ProtobufMsgHandler<ArgTypes...> {
public:
    typedef std::function<ReplyMSG(void)> Handler;

    ProtobufMsgHandlerT_Void(Disptacher &dispatcher,
                             uint16 msgtype,
                             const std::string &msgname,
                             const Handler &handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), dispatcher_(dispatcher), handler_(handler) {}

    ProtobufMsgHandlerBase::Message *newMessage() final { return new RequestMSG; }

    ProtobufMsgHandlerBase::BufPtr process(ProtobufMsgHandlerBase::MessagePtr msg, ArgTypes... args) const final {
        ReplyMSG reply = handler_();

        ProtobufMsgHandlerBase::BufPtr bufptr(new std::string);
        Disptacher::pack(*bufptr.get(), reply);
        return bufptr;
    }

private:
    Disptacher &dispatcher_;
    Handler handler_;
};

template<typename Disptacher, typename RequestMSG, typename... ArgTypes>
class ProtobufMsgHandlerT_Void<Disptacher, RequestMSG, void, ArgTypes...> : public ProtobufMsgHandler<ArgTypes...> {
public:
    typedef std::function<void(void)> Handler;

    ProtobufMsgHandlerT_Void(Disptacher &dispatcher,
                             uint16 msgtype,
                             const std::string &msgname,
                             const Handler &handler)
            : ProtobufMsgHandler<ArgTypes...>(msgtype, msgname), dispatcher_(dispatcher), handler_(handler) {}

    ProtobufMsgHandlerBase::Message *newMessage() final { return new RequestMSG; }

    ProtobufMsgHandlerBase::BufPtr process(ProtobufMsgHandlerBase::MessagePtr msg, ArgTypes... args) const final {
        handler_();
        return ProtobufMsgHandlerBase::BufPtr();
    }

private:
    Disptacher &dispatcher_;
    Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息分发器-使用协议的名字为键值
//
///////////////////////////////////////////////////////////
template<typename... ArgTypes>
class ProtobufMsgDispatcherByName {
public:
    typedef ProtobufMsgDispatcherByName<ArgTypes...> DispatcherType;
    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;

    //
    // 全局单件
    //
    static ProtobufMsgDispatcherByName<ArgTypes...> &instance() {
        static ProtobufMsgDispatcherByName<ArgTypes...> instance_;
        return instance_;
    }

    //
    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
    //
    template<typename RequestMSG, typename ReplyMSG = void>
    DispatcherType &
    on(const typename ProtobufMsgHandlerT_N<DispatcherType, RequestMSG, ReplyMSG, ArgTypes...>::Handler &handler) {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<DispatcherType, RequestMSG, ReplyMSG, ArgTypes...>(
                *this, 0, RequestMSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    template<typename RequestMSG, typename ReplyMSG = void>
    DispatcherType &
    on_void(const typename ProtobufMsgHandlerT_Void<DispatcherType, RequestMSG, ReplyMSG, ArgTypes...>::Handler &handler) {
        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<DispatcherType, RequestMSG, ReplyMSG, ArgTypes...>(
                *this, 0, RequestMSG::descriptor()->full_name(), handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // 消息分发
    //
    ProtobufMsgHandlerBase::BufPtr dispatch(const MessageHeader *msgheader, ArgTypes... args) {
        if (0 == msgheader->type_is_name || 0 == msgheader->type_len) {
            throw MsgDispatcherError("message header error");
            return ProtobufMsgHandlerBase::BufPtr();
        }

        const char *msg_name = (char *) msgheader + sizeof(MessageHeader);
        uint8 *msg_proto = (uint8 *) msg_name + msgheader->type_len;

        std::string msgname(msg_name, msgheader->type_len);

        auto it = handlers_.find(msgname);
        if (it != handlers_.end()) {
            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
            if (proto && proto->ParseFromArray(msg_proto, msgheader->size - msgheader->type_len)) {
                return it->second->process(proto, args...);
            } else {
                throw MsgDispatcherError("error body parse error : " + msgname);
            }
        } else {
            throw MsgDispatcherError("handler not exist for : " + msgname);
        }

        return ProtobufMsgHandlerBase::BufPtr();
    }

    template<typename MSG>
    static bool pack(std::string &buf, const MSG &msg) {
        return ProtobufMsgHandlerBase::packByName(buf, msg);
    }

protected:
    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler) {
        if (!handler) return false;

        if (handlers_.find(handler->msgname()) != handlers_.end()) {
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

/////////////////////////////////////////////////////////////
////
//// 消息分发器-使用协议的ID为键值)
////
//// 1.ProtobufMsgDispatcherByID1
////   Protobuf消息的约定：
////    - MSG::TYPE为消息编号
////
//// 2.ProtobufMsgDispatcherByID2
////   Protobuf消息的约定：
////    - MSG::TYPE1 为主消息编号
////    - MSG::TYPE2 为次消息编号
////
/////////////////////////////////////////////////////////////
//template <typename... ArgTypes>
//class ProtobufMsgDispatcherByID1
//{
//public:
//    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;
//
//    //
//    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
//    //
//    template <typename MSG>
//    ProtobufMsgDispatcherByID1<ArgTypes...>& on(const typename ProtobufMsgHandlerT_N<MSG, ArgTypes...>::Handler& handler)
//    {
//        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<MSG, ArgTypes...>(MSG::TYPE, MSG::descriptor()->full_name(), handler));
//        bindHandlerPtr(h);
//        return *this;
//    }
//
//    template <typename MSG>
//    ProtobufMsgDispatcherByID1<ArgTypes...>& on_void(const typename ProtobufMsgHandlerT_Void<MSG, ArgTypes...>::Handler& handler)
//    {
//        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<MSG, ArgTypes...>(MSG::TYPE, MSG::descriptor()->full_name(), handler));
//        bindHandlerPtr(h);
//        return *this;
//    }
//
//    //
//    // 消息分发
//    //
//    bool dispatch(const MessageHeader* msgheader,  ArgTypes... args)
//    {
//        if (msgheader->type_is_name > 0)
//            return false;
//
//        uint8* msg_proto = (uint8*)msgheader + sizeof(MessageHeader);
//
//        auto it = handlers_.find(msgheader->type);
//        if (it != handlers_.end())
//        {
//            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
//            if (proto && proto->ParseFromArray(msg_proto, msgheader->size))
//            {
//                it->second->process(proto, args...);
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//protected:
//    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
//    {
//        if (!handler) return false;
//
//        if (handlers_.find(handler->msgtype()) != handlers_.end()) {
//            handler->onBindFailed(__PRETTY_FUNCTION__);
//            return false;
//        }
//
//        handlers_.insert(std::make_pair(handler->msgtype(), handler));
//        return true;
//    }
//
//private:
//    typedef std::unordered_map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;
//
//    ProtobufMsgHandlerMap handlers_;
//};
//
//
//template <typename... ArgTypes>
//class ProtobufMsgDispatcherByID2
//{
//public:
//    typedef std::shared_ptr<ProtobufMsgHandler<ArgTypes...> > ProtobufMsgHandlerPtr;
//
//    //
//    // 注册消息处理函数，可以是普通函数、函数对象、成员函数、lambda等可执行体
//    //
//    template <typename MSG>
//    ProtobufMsgDispatcherByID2<ArgTypes...>& on(const typename ProtobufMsgHandlerT_N<MSG, ArgTypes...>::Handler& handler)
//    {
//        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_N<MSG, ArgTypes...>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), MSG::descriptor()->full_name(), handler));
//        bindHandlerPtr(h);
//        return *this;
//    }
//
//    template <typename MSG>
//    ProtobufMsgDispatcherByID2<ArgTypes...>& on_void(const typename ProtobufMsgHandlerT_Void<MSG, ArgTypes...>::Handler& handler)
//    {
//        ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_Void<MSG, ArgTypes...>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), MSG::descriptor()->full_name(), handler));
//        bindHandlerPtr(h);
//        return *this;
//    }
//
//    //
//    // 消息分发
//    //
//    bool dispatch(const MessageHeader* msgheader,  ArgTypes... args)
//    {
//        if (msgheader->type_is_name > 0)
//            return false;
//
//        uint8* msg_proto = (uint8*)msgheader + sizeof(MessageHeader);
//
//        auto it = handlers_.find(msgheader->type);
//        if (it != handlers_.end())
//        {
//            ProtobufMsgHandlerBase::MessagePtr proto(it->second->newMessage());
//            if (proto && proto->ParseFromArray(msg_proto, msgheader->size))
//            {
//                it->second->process(proto, args...);
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//protected:
//    bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
//    {
//        if (!handler) return false;
//
//        if (handlers_.find(handler->msgtype()) != handlers_.end()) {
//            handler->onBindFailed(__PRETTY_FUNCTION__);
//            return false;
//        }
//
//        handlers_.insert(std::make_pair(handler->msgtype(), handler));
//        return true;
//    }
//
//private:
//    typedef std::unordered_map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;
//
//    ProtobufMsgHandlerMap handlers_;
//};


#endif //TINYWORLD_MESSAGE_DISPATCHER_H
