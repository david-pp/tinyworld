#ifndef TINYWORLD_RPC_CLIENT_H
#define TINYWORLD_RPC_CLIENT_H

#include <chrono>
#include <memory>
#include <unordered_map>

#include "tinyworld.h"
#include "rpc.pb.h"


class ProtobufRPCEmitter;

struct ProtoRPCHolderBase
{
public:
    ProtoRPCHolderBase()
    {
        id_ = ++total_id_;
        createtime_ = std::chrono::high_resolution_clock::now();
    }

    long elapsed_ms()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - createtime_).count();
    }

    long elpased_ns()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - createtime_).count();
    }

    bool isTimeout()
    {
        return timeout_ms_ < 0 ? false : elapsed_ms() > timeout_ms_;
    }

    void setTimeout(long ms)
    {
        timeout_ms_ = ms;
    }

    virtual void replied(const rpc::Reply& reply) = 0;

    virtual bool pack2String(std::string& bin) = 0;

public:
    static uint64 total_id_;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

    /// 唯一标识
    uint64    id_;
    /// 创建时间点
    TimePoint createtime_;
    /// 超时时长
    long      timeout_ms_;
};

typedef std::shared_ptr<ProtoRPCHolderBase> ProtoRPCHolderPtr;

// TODO:移动到cpp文件
uint64 ProtoRPCHolderBase::total_id_ = 0;


template <typename Request, typename Reply>
struct ProtoRPCHolder : public ProtoRPCHolderBase
{
public:
    friend class ProtobufRPCEmitter;

    typedef std::function<void(const Reply&   reply)>   Callback;
    typedef std::function<void(const Request& request)> ErrorCallback;

    ProtoRPCHolder(const Request& req)
            : request_(req) {}

    ProtoRPCHolder<Request, Reply>& done(const Callback& cb)
    {
        cb_done_ = cb;
        return *this;
    }

    ProtoRPCHolder<Request, Reply>& timeout(const ErrorCallback& cb, long ms = 200)
    {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

    bool pack2String(std::string& bin) final
    {
        std::string body;
        if (request_.SerializeToString(&body))
        {
            rpc::Request req;
            req.set_id(id_);
            req.set_request(Request::descriptor()->name());
            req.set_reply(Reply::descriptor()->name());
            req.set_body(body);

            std::cout << req.DebugString() << std::endl;

            return req.SerializeToString(&bin);
        }
        return false;
    };

    void replied(const rpc::Reply& rpc_reply) final
    {
        if (rpc_reply.request() == Request::descriptor()->name()
                && rpc_reply.reply() == Reply::descriptor()->name())
        {
            Reply reply;
            if (cb_done_ && reply.ParseFromString(rpc_reply.body()))
            {
                cb_done_(reply);
            }
        }
    }

private:
    Request       request_;
    Callback      cb_done_;
    ErrorCallback cb_timeout_;
};

class ProtobufRPCEmitter
{
public:
    template <typename Request, typename Reply>
    ProtoRPCHolder<Request, Reply>& emit(const Request& request)
    {
        auto *holder = new ProtoRPCHolder<Request, Reply>(request);
        ProtoRPCHolderPtr ptr(holder);
        rpc_holders_[ptr->id_] = ptr;
        return *holder;
    }

    bool replied(const rpc::Reply& reply)
    {
        auto it = rpc_holders_.find(reply.id());
        if (it != rpc_holders_.end())
        {
            it->second->replied(reply);
            rpc_holders_.erase(it);
        }
        return false;
    }

private:
    typedef std::unordered_map<uint64, ProtoRPCHolderPtr> RPCHolderMap;
    RPCHolderMap rpc_holders_;
};

#endif //TINYWORLD_RPC_CLIENT_H
