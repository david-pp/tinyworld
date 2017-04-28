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

template<typename T>
struct TableMeta {
    typedef typename T::TableKey KeyType;

    static const char *name() {
        return T::tableName();
    }

    KeyType tableKey(const T &object) {
        return object.tableKey();
    }
};

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
    friend class TableClient;

    typedef std::function<void(const T &)> DoneCallback;
    typedef std::function<void(const KeyT &)> ErrorCallback;

    TableGetHandler(uint64_t id, RPCClientT *client, uint32_t timeout, const KeyT &key)
            : TableRequestHandlerBase(id), client_(client), timeout_ms_(timeout), key_(key) {}

    virtual ~TableGetHandler() {}

    TableGetHandler &done(const DoneCallback &callback) {
        cb_done_ = callback;

        tt::GetRequest request;
        request.set_type(TableMeta<T>::name());
        request.set_key(serialize(key_));
        client_->template emit<tt::GetRequest, tt::GetReply>(request)
                .done(std::bind(&TableGetHandler::rpc_done, this, std::placeholders::_1))
                .timeout(std::bind(&TableGetHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
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
        if (cb_done_) {
            if (reply.retcode() == 0) {
                T value;
                deserialize(value, reply.value());
                cb_done_(value);
            } else {
                if (cb_nonexist_)
                    cb_nonexist_(key_);
            }
        }

        client_->removeHandler(this->id());
    }

    void rpc_timeout(const tt::GetRequest &request) {
        if (cb_timeout_) {
            KeyT key;
            cb_timeout_(key);
        }
        client_->removeHandler(this->id());
    }

    void rpc_error(const tt::GetRequest &, rpc::ErrorCode errorCode) {
        std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
        client_->removeHandler(this->id());
    }

private:
    RPCClientT *client_;
    uint32_t timeout_ms_;
    KeyT key_;

    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;
};


template<typename T, typename RPCClientT>
class TableSetHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef std::function<void(const T &)> DoneCallback;
    typedef std::function<void(const T &)> ErrorCallback;

    TableSetHandler(uint64_t id, RPCClientT *client, uint32_t timeout, const T &value)
            : TableRequestHandlerBase(id), client_(client), timeout_ms_(timeout), value_(value) {}

    virtual ~TableSetHandler() {}

    TableSetHandler &done(const DoneCallback &callback) {
        cb_done_ = callback;

        tt::Set request;
        request.set_type(TableMeta<T>::name());
        request.set_value(serialize(value_));
        client_->template emit<tt::Set, tt::SetReply>(request)
                .done(std::bind(&TableSetHandler::rpc_done, this, std::placeholders::_1))
                .timeout(std::bind(&TableSetHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
                .error(std::bind(&TableSetHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));

        return *this;
    }

    TableSetHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::SetReply &reply) {
        if (cb_done_) {
            if (reply.retcode() == 0) {

            } else {

            }

            cb_done_(value_);
        }

        client_->removeHandler(this->id());
    }

    void rpc_timeout(const tt::Set &request) {
        if (cb_timeout_) {
            cb_timeout_(value_);
        }
        client_->removeHandler(this->id());
    }

    void rpc_error(const tt::Set &request, rpc::ErrorCode errorCode) {
        std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
        client_->removeHandler(this->id());
    }

private:
    RPCClientT *client_;
    uint32_t timeout_ms_;
    T value_;

    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
};


template<typename T, typename KeyT, typename RPCClientT>
class TableDelHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef std::function<void(const KeyT &)> DoneCallback;
    typedef std::function<void(const KeyT &)> ErrorCallback;

    TableDelHandler(uint64_t id, RPCClientT *client, uint32_t timeout, const KeyT &key)
            : TableRequestHandlerBase(id), client_(client), timeout_ms_(timeout), key_(key) {}

    virtual ~TableDelHandler() {}

    TableDelHandler &done(const DoneCallback &callback) {
        cb_done_ = callback;

        tt::Del request;
        request.set_type(TableMeta<T>::name());
        request.set_key(serialize(key_));
        client_->template emit<tt::Del, tt::DelReply>(request)
                .done(std::bind(&TableDelHandler::rpc_done, this, std::placeholders::_1))
                .timeout(std::bind(&TableDelHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
                .error(std::bind(&TableDelHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));

        return *this;
    }

    TableDelHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::DelReply &reply) {
        if (cb_done_) {
            if (reply.retcode() == 0) {

            } else {

            }
            cb_done_(key_);
        }

        client_->removeHandler(this->id());
    }

    void rpc_timeout(const tt::Del &request) {
        if (cb_timeout_) {
            cb_timeout_(key_);
        }
        client_->removeHandler(this->id());
    }

    void rpc_error(const tt::Del &, rpc::ErrorCode errorCode) {
        std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
        client_->removeHandler(this->id());
    }

private:
    RPCClientT *client_;
    uint32_t timeout_ms_;
    KeyT key_;

    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;
};


class TableClient : public AsyncRPCClient<ZMQClient> {
public:
    template<typename T>
    TableGetHandler<T, typename TableMeta<T>::KeyType, TableClient> &
    get(const typename TableMeta<T>::KeyType &key, uint32_t timeout_ms) {
        auto handler = std::make_shared<
                TableGetHandler<T,
                        typename TableMeta<T>::KeyType,
                        TableClient>>(++total_id_, this, timeout_ms, key);

        handlers_[handler->id()] = handler;
        return *handler.get();
    }

    template<typename T>
    TableSetHandler<T, TableClient> &
    set(const T &value, uint32_t timeout_ms) {
        auto handler = std::make_shared<TableSetHandler<T, TableClient>>(++total_id_, this, timeout_ms, value);
        handlers_[handler->id()] = handler;
        return *handler.get();
    }

    template<typename T>
    TableDelHandler<T, typename TableMeta<T>::KeyType, TableClient> &
    del(const typename TableMeta<T>::KeyType &key, uint32_t timeout_ms) {
        auto handler = std::make_shared<
                TableDelHandler<T,
                        typename TableMeta<T>::KeyType,
                        TableClient>>(++total_id_, this, timeout_ms, key);

        handlers_[handler->id()] = handler;
        return *handler.get();
    }

    void removeHandler(uint64_t id) {
        handlers_.erase(id);
    }

private:
    typedef std::unordered_map<uint64_t, TableRequestHandlerPtr> TableRequestHandlerMap;

    TableRequestHandlerMap handlers_;

    uint64_t total_id_ = 0;
};

#endif //TINYWORLD_TINYTABLE_H
