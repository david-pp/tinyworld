#ifndef TINYWORLD_RPC_CLIENT_H
#define TINYWORLD_RPC_CLIENT_H

#include <chrono>
#include <memory>
#include <unordered_map>
#include <iostream>

#include "tinyworld.h"
#include "rpc.pb.h"


class ProtobufRPCEmitter;

class ProtoRPCHolderBase : public std::enable_shared_from_this<ProtoRPCHolderBase>
{
public:
    ProtoRPCHolderBase(ProtobufRPCEmitter* emitter)
            : emitter_(emitter)
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

    void setTimeout(long ms);

    virtual void replied(const rpc::Reply& reply) = 0;

    virtual void timeouted() = 0;

    virtual bool pack(rpc::Request& request) = 0;

public:
    static uint64 total_id_;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

    /// 唯一标识
    uint64    id_;
    /// 创建时间点
    TimePoint createtime_;
    /// 超时时长
    long      timeout_ms_;

    /// 发射器
    ProtobufRPCEmitter* emitter_ = NULL;
};

typedef std::shared_ptr<ProtoRPCHolderBase> ProtoRPCHolderPtr;



template <typename Request, typename Reply>
class ProtoRPCHolder : public ProtoRPCHolderBase
{
public:
    friend class ProtobufRPCEmitter;

    typedef std::function<void(const Reply&   reply)>   Callback;
    typedef std::function<void(const Request& request)> TimeoutCallback;
    typedef std::function<void(const Request& request, rpc::ErrorCode errcode)> ErrorCallback;

    ProtoRPCHolder(const Request& req, ProtobufRPCEmitter* emitter)
            : ProtoRPCHolderBase(emitter), request_(req) {}

    ProtoRPCHolder<Request, Reply>& done(const Callback& cb)
    {
        cb_done_ = cb;
        return *this;
    }

    ProtoRPCHolder<Request, Reply>& timeout(const TimeoutCallback& cb, long ms = 200)
    {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

    ProtoRPCHolder<Request, Reply>& error(const ErrorCallback& cb)
    {
        cb_error_ = cb;
        return *this;
    }

    bool pack(rpc::Request& req) final
    {
        std::string body;
        if (request_.SerializeToString(&body))
        {
            req.set_id(id_);
            req.set_request(Request::descriptor()->name());
            req.set_reply(Reply::descriptor()->name());
            req.set_body(body);
            return true;
        }
        return false;
    };

    void replied(const rpc::Reply& rpc_reply) final
    {
        if (rpc_reply.errcode() != rpc::NOERROR)
        {
            if (cb_error_) cb_error_(request_, rpc_reply.errcode());
            return;
        }

        if (rpc_reply.request() != Request::descriptor()->name()
                || rpc_reply.reply() != Reply::descriptor()->name())
        {
            if (cb_error_) cb_error_(request_, rpc::REPLY_NOT_MATCHED);
            return;
        }

        Reply reply;
        if (!reply.ParseFromString(rpc_reply.body()))
        {
            if (cb_error_) cb_error_(request_, rpc::REPLY_PARSE_ERROR);
            return;
        }

        if (cb_done_)
        {
            cb_done_(reply);
        }
    }

    void timeouted() final
    {
        if (cb_timeout_)
            cb_timeout_(request_);
    }

private:
    Request         request_;
    Callback        cb_done_;
    TimeoutCallback cb_timeout_;
    ErrorCallback   cb_error_;
};

class ProtobufRPCEmitter
{
public:
    friend class ProtoRPCHolderBase;

    template <typename Request, typename Reply>
    ProtoRPCHolder<Request, Reply>& emit(const Request& request)
    {
        auto *holder = new ProtoRPCHolder<Request, Reply>(request, this);
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
            rpc_timeout_holders_.erase(it->first);
            rpc_holders_.erase(it);
            return true;
        }
        return false;
    }

    size_t checkTimeout()
    {
        size_t count = 0;
        auto it = rpc_timeout_holders_.begin();
        auto tmp = it;
        while (it != rpc_timeout_holders_.end())
        {
            tmp = it++;

            if (tmp->second->isTimeout())
            {
                tmp->second->timeouted();
                rpc_holders_.erase(tmp->first);
                rpc_timeout_holders_.erase(tmp);

                count ++;
            }
        }

        return count;
    }


private:
    typedef std::unordered_map<uint64, ProtoRPCHolderPtr> RPCHolderMap;

    RPCHolderMap rpc_holders_;
    RPCHolderMap rpc_timeout_holders_;
};


#endif //TINYWORLD_RPC_CLIENT_H
