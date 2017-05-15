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

#ifndef TINYWORLD_TINYTABLE_IN_H
#define TINYWORLD_TINYTABLE_IN_H

template<typename T>
TableGetHandler<T> &TableGetHandler<T>::done(const DoneCallback &callback) {
    cb_done_ = callback;

    tt::GetRequest request;
    request.set_type(TableMeta<T>::name());
    request.set_key(serialize(key_));
    client_->emit<tt::GetRequest, tt::GetReply>(request)
            .done(std::bind(&TableGetHandler::rpc_done, this, std::placeholders::_1))
            .timeout(std::bind(&TableGetHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
            .error(std::bind(&TableGetHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));

    return *this;
}

template<typename T>
void TableGetHandler<T>::rpc_done(const tt::GetReply &reply) {
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

template<typename T>
void TableGetHandler<T>::rpc_timeout(const tt::GetRequest &request) {
    if (cb_timeout_) {
        KeyT key;
        cb_timeout_(key);
    }
    client_->removeHandler(this->id());
}

template<typename T>
void TableGetHandler<T>::rpc_error(const tt::GetRequest &, rpc::ErrorCode errorCode) {
    std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
    client_->removeHandler(this->id());
}

///////////////////////////////////////////////////////////////////////////////////

template<typename T>
TableSetHandler<T> &TableSetHandler<T>::done(const DoneCallback &callback) {
    cb_done_ = callback;

    tt::Set request;
    request.set_type(TableMeta<T>::name());
    request.set_key(serialize(TableMeta<T>::tableKey(value_)));
    request.set_value(serialize(value_));
    client_->template emit<tt::Set, tt::SetReply>(request)
            .done(std::bind(&TableSetHandler::rpc_done, this, std::placeholders::_1))
            .timeout(std::bind(&TableSetHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
            .error(std::bind(&TableSetHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));

    return *this;
}

template<typename T>
void TableSetHandler<T>::rpc_done(const tt::SetReply &reply) {
    if (cb_done_) {
        if (reply.retcode() == 0) {

        } else {

        }

        cb_done_(value_);
    }

    client_->removeHandler(this->id());
}

template<typename T>
void TableSetHandler<T>::rpc_timeout(const tt::Set &request) {
    if (cb_timeout_) {
        cb_timeout_(value_);
    }
    client_->removeHandler(this->id());
}

template<typename T>
void TableSetHandler<T>::rpc_error(const tt::Set &request, rpc::ErrorCode errorCode) {
    std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
    client_->removeHandler(this->id());
}

///////////////////////////////////////////////////////////////////////////////////

template<typename T>
TableDelHandler<T> &TableDelHandler<T>::done(const DoneCallback &callback) {
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

template<typename T>
void TableDelHandler<T>::rpc_done(const tt::DelReply &reply) {
    if (cb_done_) {
        if (reply.retcode() == 0) {

        } else {

        }
        cb_done_(key_);
    }

    client_->removeHandler(this->id());
}

template<typename T>
void TableDelHandler<T>::rpc_timeout(const tt::Del &request) {
    if (cb_timeout_) {
        cb_timeout_(key_);
    }
    client_->removeHandler(this->id());
}

template<typename T>
void TableDelHandler<T>::rpc_error(const tt::Del &, rpc::ErrorCode errorCode) {
    std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
    client_->removeHandler(this->id());
}

///////////////////////////////////////////////////////////////////////////////////


template<typename T>
TableLoadHandler<T> &TableLoadHandler<T>::done(const DoneCallback &callback) {
    cb_done_ = callback;
    tt::LoadRequest request;
    request.set_type(TableMeta<T>::name());
    request.set_direct(direct_from_db_);
    request.set_cacheit(cache_flag_);
    if (where_.size()) request.set_where(where_);
    client_->template emit<tt::LoadRequest, tt::LoadReply>(request)
            .done(std::bind(&TableLoadHandler::rpc_done, this, std::placeholders::_1))
            .timeout(std::bind(&TableLoadHandler::rpc_timeout, this, std::placeholders::_1), timeout_ms_)
            .error(std::bind(&TableLoadHandler::rpc_error, this, std::placeholders::_1, std::placeholders::_2));
    return *this;
}

template<typename T>
void TableLoadHandler<T>::rpc_done(const tt::LoadReply &reply) {
    std::vector<T> values;
    if (deserialize(values, reply.values())) {
        cb_done_(values);
    }

    client_->removeHandler(this->id());
}

template<typename T>
void TableLoadHandler<T>::rpc_timeout(const tt::LoadRequest &request) {
    if (cb_timeout_) {
        cb_timeout_();
    }
    client_->removeHandler(this->id());
}

template<typename T>
void TableLoadHandler<T>::rpc_error(const tt::LoadRequest &request, rpc::ErrorCode errorCode) {
    std::cout << rpc::ErrorCode_Name(errorCode) << std::endl;
    client_->removeHandler(this->id());
}

///////////////////////////////////////////////////////////////////////////////////

template<typename T>
TableLoadHandler<T> &
TableClient::vLoad(uint32_t timeout_ms, bool direct, bool cacheit, const char *clause, va_list ap) {
    char statement[1024] = "";
    if (clause) {
        vsnprintf(statement, sizeof(statement), clause, ap);
    }

    auto handler = std::make_shared<TableLoadHandler<T>>(this, ++total_id_, timeout_ms, direct, statement, cacheit);
    handlers_[handler->id()] = handler;
    return *handler.get();
}

template<typename T>
TableLoadHandler<T> &TableClient::load(uint32_t timeout_ms) {
    auto handler = std::make_shared<TableLoadHandler<T>>(this, ++total_id_, timeout_ms, false, "", false);
    handlers_[handler->id()] = handler;
    return *handler.get();
}

template<typename T>
TableLoadHandler<T> &TableClient::loadFromDB(const char *clause, ...) {
    va_list ap;
    va_start(ap, clause);
    TableLoadHandler<T> &ret = vLoad<T>(1000, false, false, clause, ap);
    va_end(ap);
    return ret;
}

template<typename T>
TableLoadHandler<T> &TableClient::loadFromDBAndCache(const char *clause, ...) {
    va_list ap;
    va_start(ap, clause);
    TableLoadHandler<T> &ret = vLoad<T>(1000, false, true, clause, ap);
    va_end(ap);
    return ret;
}

#endif //TINYWORLD_TINYTABLE_IN_H
