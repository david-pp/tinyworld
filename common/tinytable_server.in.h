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

#ifndef TINYWORLD_TINYTABLE_SERVER_IN_H
#define TINYWORLD_TINYTABLE_SERVER_IN_H


template<typename T>
bool TinyDBTable<T>::processGet(const tt::Get &request, tt::GetReply &reply) {
    LOGGER_DEBUG("TinyTable", "GET:" << this->name() << ":" << request.ShortDebugString());

    KeyType key;
    if (!deserialize(key, request.key())) {
        reply.set_retcode(11);
        return false;
    }

    ObjectPtr newobj = std::make_shared<T>();
    TinyTableBase::setObjectKey(newobj, key);

    TinyORM db(this->dbpool());
    if (db.select(*newobj.get())) {
        reply.set_value(serialize(*newobj.get()));
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}

template<typename T>
bool TinyDBTable<T>::proceeSet(const tt::Set &request, tt::SetReply &reply) {
    LOGGER_DEBUG("TinyTable", "SET:" << this->name() << ":" << request.ShortDebugString());

    ObjectPtr newobj = std::make_shared<T>();
    if (!deserialize(*newobj.get(), request.value())) {
        reply.set_retcode(11);
        return false;
    }

    TinyORM db(this->dbpool());
    if (db.replace(*newobj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}

template<typename T>
bool TinyDBTable<T>::proceeDel(const tt::Del &request, tt::DelReply &reply) {

    LOGGER_DEBUG("TinyTable", "DEL:" << this->name() << ":" << request.ShortDebugString());

    KeyType key;
    if (!deserialize(key, request.key())) {
        reply.set_retcode(11);
        return false;
    }

    ObjectPtr obj = std::make_shared<T>();
    TinyTableBase::setObjectKey(obj, key);

    TinyORM db(this->dbpool());
    if (db.del(*obj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}


/////////////////////////////////////////////////////////////////////////


template<typename T>
bool TinyMemTable<T>::processGet(const tt::Get &request, tt::GetReply &reply) {
    LOGGER_DEBUG("TinyTable", "GET:" << name_ << ":" << request.ShortDebugString());

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
        reply.set_retcode(1);
        return false;
    }
}

template<typename T>
bool TinyMemTable<T>::proceeSet(const tt::Set &request, tt::SetReply &reply) {
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

    auto memobj = getObjectInMemory(TinyTableBase::getObjectKey(newobj));
    if (memobj) {
        items_.erase(TinyTableBase::getObjectKey(memobj));
        items_.insert(std::make_pair(TinyTableBase::getObjectKey(newobj), newobj));
    }

    TinyORM db(db_pool_);
    if (db.replace(*newobj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}

template<typename T>
bool TinyMemTable<T>::proceeDel(const tt::Del &request, tt::DelReply &reply) {

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
        TinyTableBase::setObjectKey(obj, key);
    }

    TinyORM db(db_pool_);
    if (db.del(*obj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}



/////////////////////////////////////////////////////////////////////////


template<typename T>
bool TinyCacheTable<T>::processGet(const tt::Get &request, tt::GetReply &reply) {
    LOGGER_DEBUG("TinyTable", "GET:" << name_ << ":" << request.ShortDebugString());

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

        TinyORM db(db_pool_);
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
}

template<typename T>
bool TinyCacheTable<T>::proceeSet(const tt::Set &request, tt::SetReply &reply) {
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

    TinyORM db(db_pool_);
    if (db.replace(*newobj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}

template<typename T>
bool TinyCacheTable<T>::proceeDel(const tt::Del &request, tt::DelReply &reply) {

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

    TinyORM db(db_pool_);
    if (db.del(*obj.get())) {
        reply.set_retcode(0);
        return true;
    } else {
        reply.set_retcode(1);
        return false;
    }
}

#endif //TINYWORLD_TINYTABLE_SERVER_IN_H
