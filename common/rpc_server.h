#ifndef TINYWORLD_RPC_SERVER_H
#define TINYWORLD_RPC_SERVER_H

#include <chrono>
#include <memory>
#include <unordered_map>
#include <functional>

#include "tinyworld.h"
#include "rpc.pb.h"

class ProtobufRPCDispatcher;

class ProtoRPCCallbackHolderBase
{
public:
    virtual rpc::Reply requested(const rpc::Request& request) = 0;

    static rpc::Reply makeReply(const rpc::Request& request)
    {
        rpc::Reply reply;
        reply.set_id(request.id());
        reply.set_request(request.request());
        reply.set_reply(request.reply());
        return reply;
    }
};

typedef std::shared_ptr<ProtoRPCCallbackHolderBase> ProtoRPCCallbackHolderPtr;

template <typename Request, typename Reply>
class ProtoRPCCallbackHolder : public ProtoRPCCallbackHolderBase
{
public:
    typedef std::function<Reply (const Request& request)> Callback;

    ProtoRPCCallbackHolder(const Callback& cb)
            : callback_(cb) {}

    rpc::Reply requested(const rpc::Request& rpc_request) final
    {
        rpc::Reply rpc_reply = ProtoRPCCallbackHolderBase::makeReply(rpc_request);

        if (rpc_request.request() == Request::descriptor()->name()
            && rpc_request.reply() == Reply::descriptor()->name()) {
            Request request;
            if (request.ParseFromString(rpc_request.body())) {
                std::string body;
                Reply reply = callback_(request);
                if (reply.SerializeToString(&body)) {
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


class ProtobufRPCDispatcher
{
public:
    template <typename Request, typename Reply>
    ProtobufRPCDispatcher& on(const typename ProtoRPCCallbackHolder<Request, Reply>::Callback& callback)
    {
        ProtoRPCCallbackHolderPtr holder(new ProtoRPCCallbackHolder<Request, Reply>(callback));
        if (callbacks_.find(Request::descriptor()->name()) == callbacks_.end())
        {
            callbacks_.insert(std::make_pair(Request::descriptor()->name(), holder));
        }
        return *this;
    }

    rpc::Reply requested(const rpc::Request& request)
    {
        auto it = callbacks_.find(request.request());
        if (it == callbacks_.end())
        {
            rpc::Reply reply = ProtoRPCCallbackHolderBase::makeReply(request);
            reply.set_errcode(rpc::REQUEST_INVALID);
            return reply;
        }

        return it->second->requested(request);
    }

private:
    typedef std::unordered_map<std::string, ProtoRPCCallbackHolderPtr> RPCCallbackMap;
    RPCCallbackMap callbacks_;
};




#endif //TINYWORLD_RPC_SERVER_H
