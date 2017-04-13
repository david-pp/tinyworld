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

//
// Message Dispatching System
//
//  - MessageBuffer : Message Streaming
//  - MessageDispatcher : Register callback and dispatch by message's type code
//  - MessageNameDispatcher : Register callback and dispatch by message's name
//

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
#include "message_helper.h"
#include "tinyserializer.h"

//
// Message Buffer: Serialize MsgT to Binary, then ompress, encrypt ...
//
//  - writeByName(msg)
//  - writeByType(msg)
//
//  MsgT Constraint : Support Serializtion
//     A. SerializerT<MsgT> is Partial Specialized
//     B. Have member functions :
//        - std::string MsgT::serialize() const;
//        - bool deserialize(const std::string &data);
//
//  Name Constraint :
//     A. MessageName<MsgT> is Partial Specialized
//     B. std::string MsgT::msgName() is Defined
//
//  Type Contraint :
//     A. MessageTypeCode<MsgT> is Partial Specialized
//     B. uint16 MsgT::msgType() is Defined
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

    template<template<typename> class SerializerT = ProtoSerializer, typename MsgT>
    bool writeByName(const MsgT &msg) {
        return packMsgByName<SerializerT, MsgT>(buf_, MessageName<MsgT>::value(), msg);
    };

    template<template<typename> class SerializerT = ProtoSerializer, typename MsgT>
    bool writeByType(const MsgT &msg) {
        return packMsgByType<SerializerT, MsgT>(buf_, MessageTypeCode<MsgT>::value(), msg);
    };

public:
    template<template<typename> class SerializerT = ProtoSerializer, typename MsgT>
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

    template<template<typename> class SerializerT = ProtoSerializer, typename MsgT>
    static bool packMsgByType(std::string &buf, uint16_t type, const MsgT &msg) {
        std::string data = serialize<SerializerT, MsgT>(msg);
        if (data.empty()) return false;

        buf.resize(sizeof(MessageHeader) + data.size());
        MessageHeader *header = (MessageHeader *) buf.data();
        {
            header->size = data.size();
            header->type_is_name = 0;
            header->type = type;
        }

        uint8_t *msg_proto = (uint8 *) (buf.data()) + sizeof(MessageHeader);
        memcpy(msg_proto, data.data(), data.size());
        return true;
    };

private:
    std::string buf_;
};

using MessageBufferPtr = std::shared_ptr<MessageBuffer>;


//
// Message Handler Base Class
//
template<typename... ArgTypes>
class MessageHandlerBase {
public:
    MessageHandlerBase(uint16 msgtype = 0, const std::string &msgname = "")
            : msgtype_(msgtype), msgname_(msgname) {}

    virtual ~MessageHandlerBase() {}

    // Process the binary message body
    //
    // Return Is:
    //  is  nullptr - no  reply
    //  not nullptr - has reply
    virtual MessageBufferPtr process(const std::string &msgbody, ArgTypes... args) const = 0;


    uint16_t msgtype() const { return msgtype_; }

    uint16_t type1() const { return MSG_TYPE_1(msgtype_); }

    uint16_t type2() const { return MSG_TYPE_2(msgtype_); }

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
    uint16_t msgtype_;
    std::string msgname_;
};

//
// Message Handler(Has Reply)
//
template<typename DispatcherT,
        typename RequestT,
        typename ReplyT,
        template<typename> class SerializerT,
        typename... ArgTypes>
class MessageHandlerT_N : public MessageHandlerBase<ArgTypes...> {
public:
    typedef std::function<ReplyT(const RequestT &msg, ArgTypes... args)> Handler;

    MessageHandlerT_N(uint16_t msgtype,
                      const std::string &msgname,
                      const Handler &handler)
            : MessageHandlerBase<ArgTypes...>(msgtype, msgname),
              handler_(handler) {}

    virtual ~MessageHandlerT_N() {}

    MessageBufferPtr process(const std::string &msgbody, ArgTypes... args) const final {
        RequestT request;
        if (deserialize<SerializerT, RequestT>(request, msgbody)) {
            ReplyT reply = handler_(request, args...);

            auto buff = std::make_shared<MessageBuffer>();
            if (DispatcherT::template write2Buffer<SerializerT, ReplyT>(*buff.get(), reply))
                return buff;
            else
                throw MsgDispatcherException("Reply write2Buffer failed");
        } else {
            throw MsgDispatcherException("deserialize faileds");
            return nullptr;
        }
    }

private:
    Handler handler_;
};

//
// Message Handler(No Reply)
//
template<typename DispatcherT,
        typename RequestT,
        template<typename> class SerializerT,
        typename... ArgTypes>
class MessageHandlerT_N<DispatcherT, RequestT, void, SerializerT, ArgTypes...>
        : public MessageHandlerBase<ArgTypes...> {
public:
    typedef std::function<void(const RequestT &msg, ArgTypes... args)> Handler;

    MessageHandlerT_N(uint16_t msgtype,
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
            throw MsgDispatcherException("deserialize faileds");
            return nullptr;
        }
    }

private:
    Handler handler_;
};


//
// Message Dispatcher By TypeCode
//
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
                MessageTypeCode<RequestT>::value(), "", handler));
        bindHandlerPtr(h);
        return *this;
    }

    //
    // Message Dispatching
    //
    MessageBufferPtr dispatch(const std::string &msgdata, ArgTypes... args) {
        MessageHeader *msgheader = (MessageHeader *) msgdata.data();

        if (msgheader->type_is_name) {
            throw MsgDispatcherException("message header error");
            return nullptr;
        }

        uint8_t *msg_body = (uint8_t *) msgheader + sizeof(MessageHeader);
        std::string msgbody((char *) msg_body, msgheader->size);

        auto it = handlers_.find(msgheader->type);
        if (it != handlers_.end()) {
            return it->second->process(msgbody, args...);
        } else {
            throw MsgDispatcherException("handler not exist for : " + std::to_string(msgheader->type));
        }

        return nullptr;
    }

    //
    // Message Streaming
    //
    template<template<typename T> class SerializerT = ProtoSerializer, typename MsgT>
    static bool write2Buffer(MessageBuffer &buff, const MsgT &msg) {
        return buff.writeByType<SerializerT, MsgT>(msg);
    }

protected:
    bool bindHandlerPtr(MessageHandlerPtr handler) {
        if (!handler) return false;

        if (handlers_.find(handler->msgtype()) != handlers_.end()) {
            handler->onBindFailed(__PRETTY_FUNCTION__);
            return false;
        }

        handlers_.insert(std::make_pair(handler->msgtype(), handler));
        return true;
    }

private:
    typedef std::unordered_map<uint16_t, MessageHandlerPtr> MessageHandlerMap;
    MessageHandlerMap handlers_;
};

//
// Message Dispatcher By Name (RECOMMENDED)
//
template<typename... ArgTypes>
class MessageNameDispatcher {
public:
    typedef MessageNameDispatcher<ArgTypes...> DispatcherType;
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
            throw MsgDispatcherException("message header error");
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
            throw MsgDispatcherException("handler not exist for : " + msgname);
        }

        return nullptr;
    }

    //
    // Message Streaming
    //
    template<template<typename T> class SerializerT = ProtoSerializer, typename MsgT>
    static bool write2Buffer(MessageBuffer &buff, const MsgT &msg) {
        return buff.writeByName<SerializerT, MsgT>(msg);
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

#endif //TINYWORLD_MESSAGE_DISPATCHER_H
