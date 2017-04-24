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

#ifndef TINYWORLD_RPC_SERVER_H
#define TINYWORLD_RPC_SERVER_H

#include <chrono>
#include <memory>
#include <unordered_map>
#include <functional>

#include "tinyworld.h"
#include "tinyrpc.pb.h"


TINY_NAMESPACE_BEGIN

class RPCDispatcher;

//
// RPC callee holder
//
class RPCCallbackBase {
public:
    virtual rpc::Reply requested(const rpc::Request &request) = 0;

    static rpc::Reply makeReply(const rpc::Request &request) {
        rpc::Reply reply;
        reply.set_id(request.id());
        reply.set_request(request.request());
        reply.set_reply(request.reply());
        return reply;
    }
};

typedef std::shared_ptr<RPCCallbackBase> RPCCallbackPtr;

template<typename Request, typename Reply>
class RPCCallback : public RPCCallbackBase {
public:
    typedef std::function<Reply(const Request &request)> Callback;

    RPCCallback(const Callback &cb)
            : callback_(cb) {}

    rpc::Reply requested(const rpc::Request &rpc_request) final {
        rpc::Reply rpc_reply = RPCCallbackBase::makeReply(rpc_request);

        if (rpc_request.request() == MessageName<Request>::value() &&
            rpc_request.reply() == MessageName<Reply>::value()) {
            Request request;
            if (deserialize(request, rpc_request.body())) {
                Reply reply = callback_(request);
                std::string body = serialize(reply);
                if (body.size()) {
                    rpc_reply.set_body(body);
                    rpc_reply.set_errcode(rpc::NOERROR);
                } else {
                    rpc_reply.set_errcode(rpc::REPLY_PACK_ERROR);
                }
            } else {
                rpc_reply.set_errcode(rpc::REQUEST_PARSE_ERROR);
            }
        } else {
            rpc_reply.set_errcode(rpc::REQUEST_NOT_MATCHED);
        }

        return rpc_reply;
    }

private:
    Callback callback_;
};

//
// RPC Dispatcher(Used by Server)
//
class RPCDispatcher {
public:
    template<typename Request, typename Reply>
    RPCDispatcher &on(const typename RPCCallback<Request, Reply>::Callback &callback) {
        auto holder = std::make_shared<RPCCallback<Request, Reply> >(callback);
        if (callbacks_.find(MessageName<Request>::value()) == callbacks_.end()) {
            callbacks_.insert(std::make_pair(MessageName<Request>::value(), holder));
        }
        return *this;
    }

    // Called by server
    rpc::Reply requested(const rpc::Request &request) {
        auto it = callbacks_.find(request.request());
        if (it == callbacks_.end()) {
            rpc::Reply reply = RPCCallbackBase::makeReply(request);
            reply.set_errcode(rpc::REQUEST_INVALID);
            return reply;
        }

        return it->second->requested(request);
    }

private:
    typedef std::unordered_map<std::string, RPCCallbackPtr> RPCCallbackMap;
    RPCCallbackMap callbacks_;
};


//
// Sync RPC Server
//
template<typename Server>
class RPCServer : public Server {
public:
    RPCServer()
            : Server(msg_dispatcher_instance_) {
        msg_dispatcher_instance_
                .on<rpc::Request>(std::bind(&RPCServer<Server>::doMsgRequest,
                                            this, std::placeholders::_1));
    }

    template<typename Request, typename Reply>
    RPCDispatcher &on(const typename RPCCallback<Request, Reply>::Callback &callback) {
        return rpc_dispatcher.on<Request, Reply>(callback);
    }

    void doMsgRequest(const rpc::Request &msg) {
        rpc::Reply rpc_rep = rpc_dispatcher.requested(msg);
        this->send(rpc_rep);
    }

private:
    RPCDispatcher rpc_dispatcher;

    MessageNameDispatcher<> msg_dispatcher_instance_;
};

//
// Async RPC Server
//
template<typename Server>
class AsyncRPCServer : public Server {
public:
    AsyncRPCServer()
            : Server(msg_dispatcher_instance_) {
        msg_dispatcher_instance_
                .on<rpc::Request>(std::bind(&AsyncRPCServer<Server>::doMsgRequest,
                                            this, std::placeholders::_1, std::placeholders::_2));
    }

    template<typename Request, typename Reply>
    RPCDispatcher &on(const typename RPCCallback<Request, Reply>::Callback &callback) {
        return rpc_dispatcher.on<Request, Reply>(callback);
    }

    void doMsgRequest(const rpc::Request &msg, const std::string &client) {
        rpc::Reply rpc_rep = rpc_dispatcher.requested(msg);
        this->send(client, rpc_rep);
    }

private:
    RPCDispatcher rpc_dispatcher;

    MessageNameDispatcher<const std::string &> msg_dispatcher_instance_;
};

TINY_NAMESPACE_END

#endif //TINYWORLD_RPC_SERVER_H
