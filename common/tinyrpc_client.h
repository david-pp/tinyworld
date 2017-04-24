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

#ifndef TINYWORLD_RPC_CLIENT_H
#define TINYWORLD_RPC_CLIENT_H

#include <chrono>
#include <memory>
#include <unordered_map>
#include <iostream>

#include "tinyworld.h"
#include "tinyrpc.pb.h"
#include "tinyserializer.h"
#include "tinyserializer_proto.h"
#include "message_dispatcher.h"

TINY_NAMESPACE_BEGIN

class RPCEmitter;

//
// RPC caller holder
//
class RPCHolderBase : public std::enable_shared_from_this<RPCHolderBase> {
public:
    RPCHolderBase(RPCEmitter *emitter)
            : emitter_(emitter) {
        id_ = ++total_id_;
        createtime_ = std::chrono::high_resolution_clock::now();
    }

    long elapsed_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - createtime_).count();
    }

    long elpased_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - createtime_).count();
    }

    bool isTimeout() {
        return timeout_ms_ < 0 ? false : elapsed_ms() > timeout_ms_;
    }

    void setTimeout(long ms);

    virtual void replied(const rpc::Reply &reply) = 0;

    virtual void timeouted() = 0;

    virtual bool pack(rpc::Request &request) = 0;

public:
    static uint64_t total_id_;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

    // unique ID
    uint64_t id_;
    // creat time point
    TimePoint createtime_;
    // timeout interval
    long timeout_ms_;
    // emitter
    RPCEmitter *emitter_ = NULL;
};

typedef std::shared_ptr<RPCHolderBase> RPCHolderPtr;


template<typename Request, typename Reply>
class RPCHolder : public RPCHolderBase {
public:
    friend class RPCEmitter;

    typedef std::function<void(const Reply &reply)> Callback;
    typedef std::function<void(const Request &request)> TimeoutCallback;
    typedef std::function<void(const Request &request, rpc::ErrorCode errcode)> ErrorCallback;

    RPCHolder(const Request &req, RPCEmitter *emitter)
            : RPCHolderBase(emitter), request_(req) {}

    RPCHolder<Request, Reply> &done(const Callback &cb) {
        cb_done_ = cb;
        return *this;
    }

    RPCHolder<Request, Reply> &timeout(const TimeoutCallback &cb, long ms = 200) {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

    RPCHolder<Request, Reply> &error(const ErrorCallback &cb) {
        cb_error_ = cb;
        return *this;
    }

    bool pack(rpc::Request &req) final {
        std::string body = serialize(request_);
        if (body.size()) {
            req.set_id(id_);
            req.set_request(MessageName<Request>::value());
            req.set_reply(MessageName<Reply>::value());
            req.set_body(body);
            return true;
        }
        return false;
    };

    void replied(const rpc::Reply &rpc_reply) final {
        if (rpc_reply.errcode() != rpc::NOERROR) {
            if (cb_error_) cb_error_(request_, rpc_reply.errcode());
            return;
        }

        if (rpc_reply.request() != MessageName<Request>::value()
            || rpc_reply.reply() != MessageName<Reply>::value()) {
            if (cb_error_) cb_error_(request_, rpc::REPLY_NOT_MATCHED);
            return;
        }

        Reply reply;
        if (!deserialize(reply, rpc_reply.body())) {
            if (cb_error_) cb_error_(request_, rpc::REPLY_PARSE_ERROR);
            return;
        }

        if (cb_done_) {
            cb_done_(reply);
        }
    }

    void timeouted() final {
        if (cb_timeout_)
            cb_timeout_(request_);
    }

private:
    Request request_;
    Callback cb_done_;
    TimeoutCallback cb_timeout_;
    ErrorCallback cb_error_;
};

//
// RPC Emitter(Used by Client)
//
class RPCEmitter {
public:
    friend class RPCHolderBase;

    template<typename Request, typename Reply>
    RPCHolder<Request, Reply> &emit(const Request &request) {
        auto *holder = new RPCHolder<Request, Reply>(request, this);
        RPCHolderPtr ptr(holder);
        rpc_holders_[ptr->id_] = ptr;
        return *holder;
    }

    // Called by client
    void replied(const rpc::Reply &reply) {
        auto it = rpc_holders_.find(reply.id());
        if (it != rpc_holders_.end()) {
            it->second->replied(reply);
            rpc_timeout_holders_.erase(it->first);
            rpc_holders_.erase(it);
        }
    }

    // Called by client
    size_t checkTimeout() {
        size_t count = 0;
        auto it = rpc_timeout_holders_.begin();
        auto tmp = it;
        while (it != rpc_timeout_holders_.end()) {
            tmp = it++;

            if (tmp->second->isTimeout()) {
                tmp->second->timeouted();
                rpc_holders_.erase(tmp->first);
                rpc_timeout_holders_.erase(tmp);

                count++;
            }
        }

        return count;
    }


private:
    typedef std::unordered_map<uint64_t, RPCHolderPtr> RPCHolderMap;

    RPCHolderMap rpc_holders_;
    RPCHolderMap rpc_timeout_holders_;
};


//
// Async RPC Client
//
template<typename Client>
class AsyncRPCClient : public Client {
public:
    AsyncRPCClient()
            : Client(msg_dispatcher_instance_) {
        msg_dispatcher_instance_
                .on<rpc::Reply>(std::bind(&RPCEmitter::replied, &rpc_emitter_, std::placeholders::_1));
    }

    template<typename Request, typename Reply>
    RPCHolder<Request, Reply> &emit(const Request &request) {
        auto &holder = rpc_emitter_.emit<Request, Reply>(request);

        rpc::Request rpc_req;
        holder.pack(rpc_req);
        this->send(rpc_req);
        return holder;
    }

    size_t checkTimeout() { return rpc_emitter_.checkTimeout(); }

private:
    RPCEmitter rpc_emitter_;

    MessageNameDispatcher<> msg_dispatcher_instance_;
};

TINY_NAMESPACE_END

#endif //TINYWORLD_RPC_CLIENT_H
