#include "redis.h"
#include "tinylogger.h"
#include "url.h"
#include "hiredis/adapters/libev.h"

namespace tiny {

template<class ReplyT>
void RedisCommand<ReplyT>::call() {
    if (redis_) {
        redis_->submitToServer(this);
    }
}

template<class ReplyT>
void RedisCommand<ReplyT>::done(void *data) {
    processReply((redisReply *) data);
}

template<class ReplyT>
void RedisCommand<ReplyT>::processReply(redisReply *r) {

    last_error_.clear();
    reply_obj_ = r;

    if (reply_obj_ == nullptr) {
        reply_status_ = ERROR_REPLY;
        last_error_ = "Received null redisReply* from hiredis.";
        LOGGER_ERROR("redis", last_error_);

        //TODO: DISCONNECT...
    } else {
        parseReplyObject();
    }

    invoke();
}


template<class ReplyT>
void RedisCommand<ReplyT>::freeReply() {

    if (reply_obj_ == nullptr)
        return;

    freeReplyObject(reply_obj_);
    reply_obj_ = nullptr;
}

/**
* Create a copy of the reply and return it. Use a guard
* to make sure we don't return a reply while it is being
* modified.
*/
template<class ReplyT>
ReplyT RedisCommand<ReplyT>::reply() {
    if (!ok()) {
        LOGGER_WARN("redis", cmd() << ": Accessing reply value while status != OK.");
    }
    return reply_val_;
}

template<class ReplyT>
std::string RedisCommand<ReplyT>::cmd() const { return AsyncRedisClient::vecToStr(cmd_); }

template<class ReplyT>
bool RedisCommand<ReplyT>::isExpectedReply(int type) {

    if (reply_obj_->type == type) {
        reply_status_ = OK_REPLY;
        return true;
    }

    if (checkErrorReply() || checkNilReply())
        return false;

    std::stringstream errorMessage;
    errorMessage << "Received reply of type " << reply_obj_->type << ", expected type " << type
                 << ".";
    last_error_ = errorMessage.str();
    LOGGER_ERROR("redis", cmd() << ": " << last_error_);
    reply_status_ = WRONG_TYPE;
    return false;
}

template<class ReplyT>
bool RedisCommand<ReplyT>::isExpectedReply(int typeA, int typeB) {

    if ((reply_obj_->type == typeA) || (reply_obj_->type == typeB)) {
        reply_status_ = OK_REPLY;
        return true;
    }

    if (checkErrorReply() || checkNilReply())
        return false;

    std::stringstream errorMessage;
    errorMessage << "Received reply of type " << reply_obj_->type << ", expected type " << typeA
                 << " or " << typeB << ".";
    last_error_ = errorMessage.str();
    LOGGER_ERROR("redis", cmd() << ": " << last_error_);
    reply_status_ = WRONG_TYPE;
    return false;
}

template<class ReplyT>
bool RedisCommand<ReplyT>::checkErrorReply() {

    if (reply_obj_->type == REDIS_REPLY_ERROR) {
        if (reply_obj_->str != 0) {
            last_error_ = reply_obj_->str;
        }

        LOGGER_ERROR("redis", cmd() << ": " << last_error_);
        reply_status_ = ERROR_REPLY;
        return true;
    }
    return false;
}

template<class ReplyT>
bool RedisCommand<ReplyT>::checkNilReply() {

    if (reply_obj_->type == REDIS_REPLY_NIL) {
        LOGGER_ERROR("redis", cmd() << ": Nil reply.");
        reply_status_ = NIL_REPLY;
        return true;
    }
    return false;
}

template<class ReplyT>
void RedisCommand<ReplyT>::on_emit(AsyncScheduler *scheduler) {
    redis_ = (AsyncRedisClient *) scheduler;

    AsyncTask::on_emit(scheduler);
}

template<class ReplyT>
AsyncTaskPtr RedisCommand<ReplyT>::clone() {
    return RedisCmd(cmd_, callback_);
}

// ----------------------------------------------------------------------------
// Specializations of parseReplyObject for all expected return types
// ----------------------------------------------------------------------------

template<>
void RedisCommand<redisReply *>::parseReplyObject() {
    if (!checkErrorReply())
        reply_status_ = OK_REPLY;
    reply_val_ = reply_obj_;
}

template<>
void RedisCommand<std::string>::parseReplyObject() {
    if (!isExpectedReply(REDIS_REPLY_STRING, REDIS_REPLY_STATUS))
        return;
    reply_val_ = {reply_obj_->str, static_cast<size_t>(reply_obj_->len)};
}

template<>
void RedisCommand<char *>::parseReplyObject() {
    if (!isExpectedReply(REDIS_REPLY_STRING, REDIS_REPLY_STATUS))
        return;
    reply_val_ = reply_obj_->str;
}

template<>
void RedisCommand<int>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_INTEGER))
        return;
    reply_val_ = (int) reply_obj_->integer;
}

template<>
void RedisCommand<long long int>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_INTEGER))
        return;
    reply_val_ = reply_obj_->integer;
}

template<>
void RedisCommand<nullptr_t>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_NIL))
        return;
    reply_val_ = nullptr;
}

template<>
void RedisCommand<std::vector<std::string>>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_ARRAY))
        return;

    for (size_t i = 0; i < reply_obj_->elements; i++) {
        redisReply *r = *(reply_obj_->element + i);
        reply_val_.emplace_back(r->str, r->len);
    }
}

template<>
void RedisCommand<std::unordered_set<std::string>>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_ARRAY))
        return;

    for (size_t i = 0; i < reply_obj_->elements; i++) {
        redisReply *r = *(reply_obj_->element + i);
        reply_val_.emplace(r->str, r->len);
    }
}

template<>
void RedisCommand<std::set<std::string>>::parseReplyObject() {

    if (!isExpectedReply(REDIS_REPLY_ARRAY))
        return;

    for (size_t i = 0; i < reply_obj_->elements; i++) {
        redisReply *r = *(reply_obj_->element + i);
        reply_val_.emplace(r->str, r->len);
    }
}

// Explicit template instantiation for available types, so that the generated
// library contains them and we can keep the method definitions out of the
// header file.
template
class RedisCommand<redisReply *>;

template
class RedisCommand<std::string>;

template
class RedisCommand<char *>;

template
class RedisCommand<int>;

template
class RedisCommand<long long int>;

template
class RedisCommand<nullptr_t>;

template
class RedisCommand<std::vector<std::string>>;

template
class RedisCommand<std::set<std::string>>;

template
class RedisCommand<std::unordered_set<std::string>>;

/////////////////////////////////////////////////////////////////////////////

static void connectCallback(const redisAsyncContext *c, int status) {
    AsyncRedisClient *client = (AsyncRedisClient *) c->data;
    if (client)
        client->onConnected(status == REDIS_OK);
}

static void disconnectCallback(const redisAsyncContext *c, int status) {
    AsyncRedisClient *client = (AsyncRedisClient *) c->data;
    if (client)
        client->onDisconnected(status == REDIS_OK);
}

struct CommandCallBackArg {
    AsyncRedisClient *client = nullptr;
    uint64_t taskid = 0;
};

// r -> redisReply
//static void cmdCallback(redisAsyncContext *c, void *r, void *privdata) {
//    CommandCallBackArg *arg = (CommandCallBackArg *) privdata;
//    if (arg && arg->client) {
//        arg->client->scheduler().triggerDone(arg->taskid, r);
//        delete arg;
//    }
//}

////////////////////////////////////////////////////////////////////////////

std::string AsyncRedisClient::vecToStr(const std::vector<std::string> &vec, const char delimiter) {
    std::string str;
    for (size_t i = 0; i < vec.size() - 1; i++)
        str += vec[i] + delimiter;
    str += vec[vec.size() - 1];
    return str;
}

std::vector<std::string> AsyncRedisClient::strToVec(const std::string &s, const char delimiter) {
    std::vector<std::string> vec;
    size_t last = 0;
    size_t next = 0;
    while ((next = s.find(delimiter, last)) != std::string::npos) {
        vec.push_back(s.substr(last, next - last));
        last = next + 1;
    }
    vec.push_back(s.substr(last));
    return vec;
}

////////////////////////////////////////////////////////////////////////////

AsyncRedisClient::AsyncRedisClient(const std::string &ip, int port, EventLoop *loop) {
    ip_ = ip;
    port_ = port;
    db_ = 0;
    evloop_ = loop;

    if (evloop_)
        evloop_->onTimer(std::bind(&AsyncRedisClient::run, this), 0.000050);
}

AsyncRedisClient::AsyncRedisClient(const std::string &urltext, EventLoop *loop) {
    context_ = NULL;
    ip_ = "";
    port_ = 0;
    db_ = 0;
    evloop_ = loop;

    TinyURL url;
    if (url.parse(urltext)) {
        ip_ = url.host;
        port_ = url.port;
        if (url.path.size())
            db_ = std::atoi(url.path.c_str());
    }

    if (evloop_)
        evloop_->onTimer(std::bind(&AsyncRedisClient::run, this), 0.000050);
}

AsyncRedisClient::~AsyncRedisClient() {
    close();
}

void AsyncRedisClient::onConnected(bool success) {
    if (success) {
        LOGGER_INFO("redis", "redis://" << ip_ << ":" << port_ << "/" << db_ << " async connected success");
    } else {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_ << " async connected failed:" << context_->errstr);
        context_ = NULL;
    }
}

void AsyncRedisClient::onDisconnected(bool success) {
    context_ = NULL;
    LOGGER_INFO("redis", "redis://" << ip_ << ":" << port_ << " async disconnected");
}

bool AsyncRedisClient::connect() {
    if (!evloop_) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_ <<
                                         " Connection error: event loop is not ready");
        return false;
    }

    context_ = redisAsyncConnect(ip_.c_str(), port_);
    if (context_ == NULL) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_ <<
                                         " Connection error: can't allocate redis context");
        return false;
    }

    if (context_->err) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_ <<
                                         " Connection error: " << context_->errstr);
        close();
        return false;
    }

    context_->data = this;

    redisLibevAttach(evloop_->evLoop(), context_);
    redisAsyncSetConnectCallback(context_, connectCallback);
    redisAsyncSetDisconnectCallback(context_, disconnectCallback);
    return true;
}

void AsyncRedisClient::checkConnection() {
    if (!context_)
        connect();
}

void AsyncRedisClient::run() {
    AsyncScheduler::run();
}

void AsyncRedisClient::close() {
    if (context_) {
        redisAsyncDisconnect(context_);
        context_ = NULL;
    }
}



//bool AsyncRedisClient::command(myredis::RPC *callback, const char *format, ...) {
//    bool ret = false;
//    va_list ap;
//    va_start(ap, format);
//    ret = command_v(callback, format, ap);
//    va_end(ap);
//    return ret;
//}
//
//
//bool AsyncRedisClient::command_v(myredis::RPC *callback, const char *format, va_list ap) {
//    if (!callback) return false;
//
//    checkConnection();
//    if (!isConnected())
//        return false;
//
//    callback->client = this;
//
//    CommandCallBackArg *arg = new CommandCallBackArg;
//    arg->scheduler = &this->scheduler_;
//    arg->rpcid = callback->id;
//
//    redisvAsyncCommand(context_, cmdCallback, arg, format, ap);
//    return true;
//}
//
//
//bool AsyncRedisClient::command_argv(myredis::RPC *callback, const std::vector<std::string> &args) {
//    if (!callback) return false;
//
//    checkConnection();
//    if (!isConnected())
//        return false;
//
//    callback->client = this;
//
//    CommandCallBackArg *arg = new CommandCallBackArg;
//    arg->scheduler = &this->scheduler_;
//    arg->rpcid = callback->id;
//
//    int argc = (int) args.size();
//    char **argv = new char *[args.size()];
//    size_t *argvlen = new size_t[args.size()];
//    for (size_t i = 0; i < args.size(); i++) {
//        argv[i] = (char *) args[i].data();
//        argvlen[i] = args[i].size();
//    }
//
//    redisAsyncCommandArgv(context_, cmdCallback, arg, argc, (const char **) argv, argvlen);
//
//    delete[] argv;
//    delete[] argvlen;
//    return true;
//}
//
//

//bool AsyncRedisClient::call(myredis::RPC *rpc, const ContextPtr &context, int timeoutms) {
//    if (!rpc) return false;
//
//    rpc->client = this;
//    scheduler_.call(rpc, context, timeoutms);
//    return true;
//}
//
//bool AsyncRedisClient::callBySerials(SerialsCallBack *cb, const AsyncRPC::PtrArray &rpcs, const ContextPtr &context,
//                                     int timeoutms) {
//    if (cb) {
//        for (size_t i = 0; i < rpcs.size(); i++) {
//            myredis::RPC *child = (myredis::RPC *) rpcs[i];
//            child->client = this;
//        }
//
//        return scheduler_.callBySerials(cb, rpcs, context, timeoutms);
//    }
//    return false;
//}
//
//bool AsyncRedisClient::callByParallel(ParallelCallBack *cb, const AsyncRPC::PtrArray &rpcs, const ContextPtr &context,
//                                      int timeoutms) {
//    if (cb) {
//        for (size_t i = 0; i < rpcs.size(); i++) {
//            myredis::RPC *child = (myredis::RPC *) rpcs[i];
//            child->client = this;
//        }
//        return scheduler_.callByParallel(cb, rpcs, context, timeoutms);
//    }
//    return false;
//}
//
//
//static void cmdCallback2(redisAsyncContext *c, void *r, void *privdata) {
//    myredis::Callback *cb = (myredis::Callback *) privdata;
//    if (cb) {
//        RedisReply reply;
//        reply.parseFrom((redisReply *) r);
//        cb->replied(reply);
//        delete cb;
//    }
//}
//
//bool AsyncRedisClient::cmd(myredis::Callback *cb, const char *format, ...) {
//    bool ret = false;
//    va_list ap;
//    va_start(ap, format);
//    ret = cmd_v(cb, format, ap);
//    va_end(ap);
//    return ret;
//}
//
//bool AsyncRedisClient::cmd_v(myredis::Callback *cb, const char *format, va_list ap) {
//    if (!cb) return false;
//
//    checkConnection();
//    if (!isConnected())
//        return false;
//
//    if (cb) {
//        cb->client = this;
//    }
//
//    redisvAsyncCommand(context_, cmdCallback2, cb, format, ap);
//}
//
//bool AsyncRedisClient::cmd_argv(myredis::Callback *cb, const std::vector<std::string> &args) {
//    if (!cb) return false;
//
//    checkConnection();
//    if (!isConnected())
//        return false;
//
//    int argc = (int) args.size();
//    char **argv = new char *[args.size()];
//    size_t *argvlen = new size_t[args.size()];
//    for (size_t i = 0; i < args.size(); i++) {
//        argv[i] = (char *) args[i].data();
//        argvlen[i] = args[i].size();
//    }
//
//    redisAsyncCommandArgv(context_, cmdCallback2, cb, argc, (const char **) argv, argvlen);
//
//    delete[] argv;
//    delete[] argvlen;
//
//    return true;
//}

} // namespace tiny