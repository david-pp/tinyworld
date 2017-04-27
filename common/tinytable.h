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

#ifndef TINYWORLD_TINYTABLE_H
#define TINYWORLD_TINYTABLE_H

#include "tinylogger.h"
#include "tinyrpc_client.h"
#include "zmq_client.h"
#include "tinytable.pb.h"

class TableClient;

class TableRequestHandlerBase {
public:
    TableRequestHandlerBase(uint64_t id) : id_(id) {}

    uint64_t id() const { return id_; }

private:
    uint64_t id_;
};

typedef std::shared_ptr<TableRequestHandlerBase> TableRequestHandlerPtr;

template<typename T, typename KeyT, typename RPCClientT>
class TableGetHandler : public TableRequestHandlerBase {
public:
    typedef std::function<void(const T &)> DoneCallback;
    typedef std::function<void(const KeyT &)> ErrorCallback;

    TableGetHandler(uint64_t id, RPCClientT *client)
            : TableRequestHandlerBase(id), client_(client) {}

    virtual ~TableGetHandler() {}

    TableGetHandler &done(const DoneCallback &callback) {
        cb_done_ = callback;

        tt::GetRequest request;
        request.set_type(typeid(T).name());
        request.set_keys("1235");
        client_->template emit<tt::GetRequest, tt::GetReply>(request)
                .done(std::bind(&TableGetHandler::rpc_done, this, std::placeholders::_1))
                .timeout(std::bind(&TableGetHandler::rpc_timeout, this, std::placeholders::_1))
                .error(std::bind(&TableGetHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));

        return *this;
    }

    TableGetHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

    TableGetHandler &nonexist(const ErrorCallback &callback) {
        cb_nonexist_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::GetReply &reply) {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        if (cb_done_) {
            T value;
            cb_done_(value);
        }
    }

    void rpc_timeout(const tt::GetRequest &request) {
        if (cb_timeout_) {
            KeyT key;
            cb_timeout_(key);
        }
    }

    void rpc_error(const tt::GetRequest &, rpc::ErrorCode errorCode) {
        std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
    }

private:
    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;

    RPCClientT *client_;
};

class TableClient : public AsyncRPCClient<ZMQClient> {
public:
    template<typename T, typename KeyT>
    TableGetHandler<T, KeyT, TableClient> &get(const KeyT &key, uint32_t timeout_ms = 200) {
        auto handler = std::make_shared<TableGetHandler<T, KeyT, TableClient>>(++total_id_, this);
        handlers_[handler->id()] = handler;
        return *handler.get();
    }

private:
    typedef std::unordered_map<uint64_t, TableRequestHandlerPtr> TableRequestHandlerMap;

    TableRequestHandlerMap handlers_;

    uint64_t total_id_ = 0;
};

#endif //TINYWORLD_TINYTABLE_H
