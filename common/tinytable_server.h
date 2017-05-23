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

#ifndef TINYWORLD_TINYTABLE_SERVER_H
#define TINYWORLD_TINYTABLE_SERVER_H

#include <unordered_map>
#include <memory>
#include "tinylogger.h"
#include "tinytable.h"
#include "tinytable.pb.h"
#include "tinyorm.h"
#include "tinyorm_mysql.h"
#include "tinyrpc_server.h"
#include "zmq_server.h"

TINY_NAMESPACE_BEGIN

class TinyTableFactory;

class TinyTableBase {
public:
    TinyTableBase(const std::string &name, MySqlConnectionPool *pool)
            : name_(name), db_pool_(pool) {}

    virtual bool processGet(const tt::Get &request, tt::GetReply &reply) = 0;

    virtual bool proceeSet(const tt::Set &request, tt::SetReply &reply) = 0;

    virtual bool proceeDel(const tt::Del &request, tt::DelReply &reply) = 0;

    const std::string &name() { return name_; }

    MySqlConnectionPool *dbpool() { return db_pool_; }

    template<typename T>
    static void setObjectKey(std::shared_ptr<T> object, const typename TableMeta<T>::KeyType &key) {
        TableMeta<T>::setTableKey(*object.get(), key);
    }

    template<typename T>
    static typename TableMeta<T>::KeyType getObjectKey(std::shared_ptr<T> object) {
        return TableMeta<T>::tableKey(*object.get());
    }

protected:
    std::string name_;
    MySqlConnectionPool *db_pool_;
};

typedef std::shared_ptr<TinyTableBase> TinyTablePtr;


template<typename T>
class TinyDBTable : public TinyTableBase {
public:
    typedef typename TableMeta<T>::KeyType KeyType;
    typedef std::shared_ptr<T> ObjectPtr;

    TinyDBTable(const std::string &name, MySqlConnectionPool *pool)
            : TinyTableBase(name, pool) {}

    bool processGet(const tt::Get &request, tt::GetReply &reply) final;

    bool proceeSet(const tt::Set &request, tt::SetReply &reply) final;

    bool proceeDel(const tt::Del &request, tt::DelReply &reply) final;
};


template<typename T>
class TinyMemTable : public TinyTableBase {
public:
    typedef typename TableMeta<T>::KeyType KeyType;
    typedef std::shared_ptr<T> ObjectPtr;

    TinyMemTable(const std::string &name, MySqlConnectionPool *pool)
            : TinyTableBase(name, pool) {}

    ObjectPtr getObjectInMemory(const KeyType &key) {
        auto it = items_.find(key);
        if (it != items_.end())
            return it->second;
        return nullptr;
    }

    bool processGet(const tt::Get &request, tt::GetReply &reply) final;

    bool proceeSet(const tt::Set &request, tt::SetReply &reply) final;

    bool proceeDel(const tt::Del &request, tt::DelReply &reply) final;

private:
    std::unordered_map<KeyType, ObjectPtr> items_;
};

template<typename T>
class TinyCacheTable : public TinyTableBase {
public:
    typedef typename TableMeta<T>::KeyType KeyType;
    typedef std::shared_ptr<T> ObjectPtr;

    TinyCacheTable(const std::string &name, MySqlConnectionPool *pool)
            : TinyTableBase(name, pool) {}

    bool processGet(const tt::Get &request, tt::GetReply &reply) final;

    bool proceeSet(const tt::Set &request, tt::SetReply &reply) final;

    bool proceeDel(const tt::Del &request, tt::DelReply &reply) final;

public:
    ObjectPtr getObjectInMemory(const KeyType &key) {
        auto it = items_.find(key);
        if (it != items_.end())
            return it->second;
        return nullptr;
    }

    void setKey(ObjectPtr object, const KeyType &key) {
        TableMeta<T>::setTableKey(*object.get(), key);
    }

    KeyType getKey(ObjectPtr object) {
        return TableMeta<T>::tableKey(*object.get());
    }

private:
    std::unordered_map<KeyType, ObjectPtr> items_;
};


class TinyTableFactory {
public:
    static TinyTableFactory &instance() {
        static TinyTableFactory factory;
        return factory;
    }

    void connectDB(const std::string &url) {
        mysql_.connect(url);
    }

    template<typename T>
    TinyTableFactory &dbTable() {
        std::string name = TableMeta<T>::name();
        TinyTablePtr ptr = std::make_shared<TinyDBTable<T>>(name, &mysql_);
        tables_.insert(std::make_pair(name, ptr));
        return *this;
    }

    template<typename T>
    TinyTableFactory &memTable() {
        std::string name = TableMeta<T>::name();
        TinyTablePtr ptr = std::make_shared<TinyMemTable<T>>(name, &mysql_);
        tables_.insert(std::make_pair(name, ptr));
        return *this;
    }

    template<typename T>
    TinyTableFactory &cacheTable(uint32_t expired_ms = static_cast<uint32_t >(-1)) {
        std::string name = TableMeta<T>::name();
        TinyTablePtr ptr = std::make_shared<TinyCacheTable<T>>(name, &mysql_);
        tables_.insert(std::make_pair(name, ptr));
        return *this;
    }

public:
    bool processGet(const tt::Get &request, tt::GetReply &reply) {
        auto table = getTableByName(request.type());
        if (table)
            return table->processGet(request, reply);
        return false;
    }

    bool proceeSet(const tt::Set &request, tt::SetReply &reply) {
        auto table = getTableByName(request.type());
        if (table)
            return table->proceeSet(request, reply);
        return false;
    }

    bool proceeDel(const tt::Del &request, tt::DelReply &reply) {
        auto table = getTableByName(request.type());
        if (table)
            return table->proceeDel(request, reply);
        return false;
    }

    TinyTablePtr getTableByName(const std::string &name) {
        auto it = tables_.find(name);
        if (it != tables_.end())
            return it->second;
        return nullptr;
    }

private:
    typedef std::unordered_map<std::string, TinyTablePtr> TinyTableMap;
    TinyTableMap tables_;

    MySqlConnectionPool mysql_;
};

class TableServer : public AsyncRPCServer<ZMQAsyncServer> {
public:
    TableServer(TinyTableFactory *factory = &TinyTableFactory::instance())
            : factory_(factory) {

        on<tt::Get, tt::GetReply>([this](const tt::Get &request) {
            tt::GetReply reply;
            reply.set_type(request.type());
            reply.set_key(request.key());
            this->factory_->processGet(request, reply);
            return reply;
        });

        on<tt::Set, tt::SetReply>([this](const tt::Set &request) {
            tt::SetReply reply;
            reply.set_type(request.type());
            this->factory_->proceeSet(request, reply);
            return reply;
        });

        on<tt::Del, tt::DelReply>([this](const tt::Del &request) {
            tt::DelReply reply;
            reply.set_type(request.type());
            this->factory_->proceeDel(request, reply);
            return reply;
        });
    }

private:
    TinyTableFactory *factory_ = nullptr;
};

#include "tinytable_server.in.h"

TINY_NAMESPACE_END

#endif //TINYWORLD_TINYTABLE_SERVER_H
