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

class TinyTableBase {
public:
    virtual bool processGet(const tt::Get &request, tt::GetReply &reply) = 0;

    virtual bool proceeSet(const tt::Set &request, tt::SetReply &reply) = 0;

    virtual bool proceeDel(const tt::Del &request, tt::DelReply &reply) = 0;

protected:
    std::string name_;
};

typedef std::shared_ptr<TinyTableBase> TinyTablePtr;

template<typename T>
class TinyTable : public TinyTableBase {
public:
    typedef typename TableMeta<T>::KeyType KeyType;
    typedef std::shared_ptr<T> ObjectPtr;

    bool processGet(const tt::Get &request, tt::GetReply &reply) final {
        LOGGER_DEBUG("TinyTable", "GET:" << name_ << ":" << request.ShortDebugString());

        reply.set_type(request.type());
        reply.set_key(request.key());

        KeyType key;
        if (!deserialize(key, request.key())) {
            reply.set_retcode(11);
            return false;
        }

        auto memobj = getObjectInMemory(key);

        if (memobj) {  // hit
            LOGGER_DEBUG("TinyTable", "GET:" << name_ << ":" << "HIT!!");

            reply.set_value(serialize(*memobj.get()));
            reply.set_retcode(0);
            return true;
        } else { // miss
            LOGGER_DEBUG("TinyTable", "GET:" << name_ << ":" << "MISS!!");

            ObjectPtr newobj = std::make_shared<T>();
            setKey(newobj, key);

            TinyORM db;
            if (db.select(*newobj.get())) {
                reply.set_value(serialize(*newobj.get()));
                reply.set_retcode(0);
                items_.insert(std::make_pair(getKey(newobj), newobj));
                return true;
            } else {
                reply.set_retcode(1);
                return false;
            }
        }
        return true;
    }

    bool proceeSet(const tt::Set &request, tt::SetReply &reply) final {
        LOGGER_DEBUG("TinyTable", "SET:" << name_ << ":" << request.ShortDebugString());

        reply.set_type(request.type());

        ObjectPtr newobj = std::make_shared<T>();
        try {
            if (!deserialize(*newobj.get(), request.value())) {
                reply.set_retcode(11);
                return false;
            }
        } catch (const std::exception &e) {
            LOGGER_ERROR("TinyTable", e.what());
        }

        auto memobj = getObjectInMemory(getKey(newobj));
        if (memobj) {
            items_.erase(getKey(memobj));
            items_.insert(std::make_pair(getKey(newobj), newobj));
        }

        TinyORM db;
        if (db.replace(*newobj.get())) {
            reply.set_retcode(0);
            return true;
        } else {
            reply.set_retcode(1);
            return false;
        }
        return true;
    }

    bool proceeDel(const tt::Del &request, tt::DelReply &reply) final {

        LOGGER_DEBUG("TinyTable", "DEL:" << name_ << ":" << request.ShortDebugString());

        reply.set_type(request.type());
        reply.set_key(request.key());

        KeyType key;
        if (!deserialize(key, request.key())) {
            reply.set_retcode(11);
            return false;
        }

        ObjectPtr obj = nullptr;
        ObjectPtr memobj = getObjectInMemory(key);
        if (memobj) {
            obj = memobj;
        } else {
            obj = std::make_shared<T>();
            setKey(obj, key);
        }

        TinyMySqlORM db;
        if (db.del(*obj.get())) {
            reply.set_retcode(0);
            return true;
        } else {
            reply.set_retcode(1);
            return false;
        }

        return true;
    }

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

class TableServer {
};


TINY_NAMESPACE_END

#endif //TINYWORLD_TINYTABLE_SERVER_H
