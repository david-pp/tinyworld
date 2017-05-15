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

TINY_NAMESPACE_BEGIN

template<typename T>
struct TableMeta {
    typedef typename T::TableKey KeyType;

    static const char *name() {
        return T::tableName();
    }

    static KeyType tableKey(const T &object) {
        return object.tableKey();
    }
};

class TableClient;

class TableRequestHandlerBase {
public:
    TableRequestHandlerBase(TableClient *client, uint64_t id, uint32_t timeout)
            : client_(client), id_(id), timeout_ms_(timeout) {}

    uint64_t id() const { return id_; }

protected:
    TableClient *client_;
    uint64_t id_;
    uint32_t timeout_ms_;
};

typedef std::shared_ptr<TableRequestHandlerBase> TableRequestHandlerPtr;

//
// Get Request Handler
//
template<typename T>
class TableGetHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef typename TableMeta<T>::KeyType KeyT;
    typedef std::function<void(const T &)> DoneCallback;
    typedef std::function<void(const KeyT &)> ErrorCallback;

    TableGetHandler(TableClient *client, uint64_t id, uint32_t timeout, const KeyT &key)
            : TableRequestHandlerBase(client, id, timeout), key_(key) {}

    virtual ~TableGetHandler() {}

    TableGetHandler &done(const DoneCallback &callback);

    TableGetHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

    TableGetHandler &nonexist(const ErrorCallback &callback) {
        cb_nonexist_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::GetReply &reply);

    void rpc_timeout(const tt::GetRequest &request);

    void rpc_error(const tt::GetRequest &, rpc::ErrorCode errorCode);

private:
    KeyT key_;
    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;
};


//
// Set Request Handler
//
template<typename T>
class TableSetHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef typename TableMeta<T>::KeyType KeyT;
    typedef std::function<void(const T &)> DoneCallback;
    typedef std::function<void(const T &)> ErrorCallback;

    TableSetHandler(TableClient *client, uint64_t id, uint32_t timeout, const T &value)
            : TableRequestHandlerBase(client, id, timeout), value_(value) {}

    virtual ~TableSetHandler() {}

    TableSetHandler &done(const DoneCallback &callback);

    TableSetHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::SetReply &reply);

    void rpc_timeout(const tt::Set &request);

    void rpc_error(const tt::Set &request, rpc::ErrorCode errorCode);

private:
    T value_;
    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
};


//
// Delete Request Handler
//
template<typename T>
class TableDelHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef typename TableMeta<T>::KeyType KeyT;
    typedef std::function<void(const KeyT &)> DoneCallback;
    typedef std::function<void(const KeyT &)> ErrorCallback;

    TableDelHandler(TableClient *client, uint64_t id, uint32_t timeout, const KeyT &key)
            : TableRequestHandlerBase(client, id, timeout), key_(key) {}

    virtual ~TableDelHandler() {}

    TableDelHandler &done(const DoneCallback &callback);

    TableDelHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::DelReply &reply);

    void rpc_timeout(const tt::Del &request);

    void rpc_error(const tt::Del &, rpc::ErrorCode errorCode);

private:
    KeyT key_;

    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;
};


//
// Load data from database/cache
//
template<typename T>
class TableLoadHandler : public TableRequestHandlerBase {
public:
    friend class TableClient;

    typedef std::function<void(const std::vector<T> &value)> DoneCallback;
    typedef std::function<void()> ErrorCallback;

    TableLoadHandler(TableClient *client, uint64_t id, uint32_t timeout, bool direct = true,
                     const std::string &where = "", bool cache = false)
            : TableRequestHandlerBase(client, id, timeout), direct_from_db_(direct), where_(where),
              cache_flag_(cache) {}

    virtual ~TableLoadHandler() {}

    TableLoadHandler &done(const DoneCallback &callback);

    TableLoadHandler &timeout(const ErrorCallback &callback) {
        cb_timeout_ = callback;
        return *this;
    }

    TableLoadHandler &error(const ErrorCallback &callback) {
        cb_error_ = callback;
        return *this;
    }

protected:
    void rpc_done(const tt::LoadReply &reply);

    void rpc_timeout(const tt::LoadRequest &request);

    void rpc_error(const tt::LoadRequest &request, rpc::ErrorCode errorCode);

private:
    bool direct_from_db_;
    std::string where_;
    bool cache_flag_;
    DoneCallback cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_error_;
};


class TableClient : public AsyncRPCClient<ZMQClient> {
public:
    //
    // get / set / del
    //
    template<typename T>
    TableGetHandler<T> &
    get(const typename TableMeta<T>::KeyType &key, uint32_t timeout_ms) {
        auto handler = std::make_shared<TableGetHandler<T>>(this, ++total_id_, timeout_ms, key);
        handlers_[handler->id()] = handler;
        return *handler.get();
    }

    template<typename T>
    TableSetHandler<T> &
    set(const T &value, uint32_t timeout_ms) {
        auto handler = std::make_shared<TableSetHandler<T>>(this, ++total_id_, timeout_ms, value);
        handlers_[handler->id()] = handler;
        return *handler.get();
    }

    template<typename T>
    TableDelHandler<T> &
    del(const typename TableMeta<T>::KeyType &key, uint32_t timeout_ms) {
        auto handler = std::make_shared<TableDelHandler<T>>(this, ++total_id_, timeout_ms, key);
        handlers_[handler->id()] = handler;
        return *handler.get();
    }


    //
    // load data from cache
    //
    template<typename T>
    TableLoadHandler<T> &load(uint32_t timeout_ms);

    //
    // load data from database
    //
    template<typename T>
    TableLoadHandler<T> &loadFromDB(const char *clause, ...);

    template<typename T>
    TableLoadHandler<T> &loadFromDBAndCache(const char *clause, ...);


public:
    void removeHandler(uint64_t id) {
        handlers_.erase(id);
    }

    template<typename T>
    TableLoadHandler<T> &vLoad(uint32_t timeout_ms, bool direct, bool cacheit, const char *clause, va_list ap);

private:
    typedef std::unordered_map<uint64_t, TableRequestHandlerPtr> TableRequestHandlerMap;

    TableRequestHandlerMap handlers_;
    uint64_t total_id_ = 0;
};

#include "tinytable.in.h"

TINY_NAMESPACE_END

#endif //TINYWORLD_TINYTABLE_H
