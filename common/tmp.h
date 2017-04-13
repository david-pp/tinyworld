//
// Created by wangdawei on 2017/4/13.
//

#ifndef TINYWORLD_TMP_H
#define TINYWORLD_TMP_H

#include "tinyworld.h"

#include <map>
#include <cstring>
#include <unordered_map>
#include <tuple>
#include <iostream>
#include <memory>
#include <functional>
#include <exception>
#include <google/protobuf/message.h>

#include "message.h"
#include "message_helper.h"
// TODO: delete following after refactor


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
            throw MsgDispatcherException("message header error");
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
                throw MsgDispatcherException("error body parse error : " + msgname);
            }
        } else {
            throw MsgDispatcherException("handler not exist for : " + msgname);
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


#endif //TINYWORLD_TMP_H
