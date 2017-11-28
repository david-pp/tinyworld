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

#ifndef TINYWORLD_REDIS_H
#define TINYWORLD_REDIS_H

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <hiredis/async.h>
#include "async.h"
#include "eventloop.h"
#include "tinylogger.h"

namespace tiny {

class AsyncRedisClient;

template<class ReplyT>
class RedisCommand : public AsyncTask {
public:
    friend class AsyncRedisClient;

    // Reply codes
    static const int NO_REPLY = -1;   // No reply yet
    static const int OK_REPLY = 0;    // Successful reply of the expected type
    static const int NIL_REPLY = 1;   // Got a nil reply
    static const int ERROR_REPLY = 2; // Got an error reply
    static const int SEND_ERROR = 3;  // Could not send to server
    static const int WRONG_TYPE = 4;  // Got reply, but it was not the expected type
    static const int TIMEOUT = 5;     // No reply, timed out

public:
    RedisCommand() {}

    RedisCommand(AsyncRedisClient *redis, const std::vector<std::string> &cmd,
                 const std::function<void(RedisCommand<ReplyT> &)> &callback)
            : redis_(redis),
              cmd_(cmd), callback_(callback) {

    }

    AsyncTaskPtr clone() override;

    void call() override;

    void done(void *data) override;

    /**
    * Returns the reply status of this command.
    */
    int status() const { return reply_status_; }

    std::string lastError() const { return last_error_; }

    /**
    * Returns true if this command got a successful reply.
    */
    bool ok() const { return reply_status_ == OK_REPLY; }

    /**
    * Returns the reply value, if the reply was successful (ok() == true).
    */
    ReplyT reply();

    /**
    * Returns the command string represented by this object.
    */
    std::string cmd() const;


    // Allow public access to constructed data
    AsyncRedisClient *redis_ = nullptr;

    std::vector<std::string> cmd_;


    // Handles a new reply from the server
    void processReply(redisReply *r);

    // Invoke a user callback from the reply object. This method is specialized
    // for each ReplyT of RedisCommand.
    void parseReplyObject();

    // Directly invoke the user callback if it exists
    void invoke() {
        if (callback_)
            callback_(*this);
    }

    bool checkErrorReply();

    bool checkNilReply();

    bool isExpectedReply(int type);

    bool isExpectedReply(int typeA, int typeB);


    // If needed, free the redisReply
    void freeReply();

    // The last server reply
    redisReply *reply_obj_ = nullptr;

    // User callback
    const std::function<void(RedisCommand<ReplyT> &)> callback_;

    // Place to store the reply value and status.
    ReplyT reply_val_;
    int reply_status_;
    std::string last_error_;


    void on_emit(AsyncScheduler *scheduler) override;

private:
    RedisCommand(const RedisCommand &) = delete;

    RedisCommand &operator=(const RedisCommand &) = delete;
};

template<typename ReplyT>
AsyncTaskPtr
RedisCmd(std::vector<std::string> cmd, const std::function<void(RedisCommand<ReplyT> &)> callback = nullptr) {
    if (cmd.empty())
        return nullptr;

    return std::make_shared<RedisCommand<ReplyT>>(nullptr, cmd, callback);
}


class AsyncRedisClient : public AsyncScheduler {
public:
    AsyncRedisClient(const std::string &ip, int port, EventLoop *loop = EventLoop::instance());

    AsyncRedisClient(const std::string &url, EventLoop *loop = EventLoop::instance());

    ~AsyncRedisClient();

    bool isConnected() { return context_ && context_->err == 0; }

    bool connect();

    void close();

    void run();


//    // RPC MODE
//    bool call(myredis::RPC *rpc, const ContextPtr &context = ContextPtr(), int timeoutms = -1);
//
//    bool
//    callBySerials(SerialsCallBack *cb, const AsyncRPC::PtrArray &rpcs, const ContextPtr &context, int timeoutms = -1);
//
//    bool
//    callByParallel(ParallelCallBack *cb, const AsyncRPC::PtrArray &rpcs, const ContextPtr &context, int timeoutms = -1);
//
//    // COMMAND MODE
//    bool cmd(myredis::Callback *cb, const char *format, ...);
//
//    bool cmd_v(myredis::Callback *cb, const char *format, va_list ap);
//
//    bool cmd_argv(myredis::Callback *cb, const std::vector<std::string> &args);

    template<typename ReplyT>
    AsyncTaskPtr Command(std::vector<std::string> cmd, const std::function<void(RedisCommand<ReplyT> &)> callback) {
        if (cmd.empty())
            return nullptr;

        return std::make_shared<RedisCommand<ReplyT>>(this, cmd, callback);
    }

public:
    template<class ReplyT>
    void cmd(const std::vector<std::string> &cmd,
             const std::function<void(RedisCommand<ReplyT> &)> &callback = nullptr) {
        emit(Command(cmd, callback));
    }

//
//    bool command(myredis::RPC *callback, const char *format, ...);
//
//    bool command_v(myredis::RPC *callback, const char *format, va_list ap);
//
//    bool command_argv(myredis::RPC *callback, const std::vector<std::string> &args);

    void onConnected(bool success);

    void onDisconnected(bool success);


    static std::string vecToStr(const std::vector<std::string> &vec, const char delimiter = ' ');

    static std::vector<std::string> strToVec(const std::string &s, const char delimiter = ' ');


    // Submit an asynchronous command to the Redox server. Return
    // true if succeeded, false otherwise.
    template<typename ReplyT>
    bool submitToServer(RedisCommand<ReplyT> *c) {

        checkConnection();

        // Construct a char** from the vector
        std::vector<const char *> argv;
        std::transform(c->cmd_.begin(), c->cmd_.end(), back_inserter(argv),
                       [](const std::string &s) {
                           return s.c_str();
                       });

        // Construct a size_t* of string lengths from the vector
        std::vector<size_t> argvlen;
        transform(c->cmd_.begin(), c->cmd_.end(), back_inserter(argvlen),
                  [](const std::string &s) {
                      return s.size();
                  });

        if (redisAsyncCommandArgv(context_, commandCallback < ReplyT > , (void *) c->id_, argv.size(),
                                  &argv[0], &argvlen[0]) != REDIS_OK) {
            LOGGER_ERROR("redis", "Could not send \"" << c->cmd() << "\": " << context_->errstr);
            c->reply_status_ = RedisCommand<ReplyT>::SEND_ERROR;
            c->invoke();
            return false;
        }

        return true;
    }

    // Callback given to hiredis to invoke when a reply is received
    template<class ReplyT>
    static void commandCallback(redisAsyncContext *ctx, void *r, void *privdata) {
        AsyncRedisClient *redis = (AsyncRedisClient *) ctx->data;
        uint64_t id = (uint64_t) privdata;
        redisReply *reply_obj = (redisReply *) r;
        if (!redis->triggerDone(id, reply_obj)) {
            freeReplyObject(reply_obj);
            return;
        }
    }

private:
    void checkConnection();

    redisAsyncContext *context_ = nullptr;

    EventLoop *evloop_ = nullptr;

    std::string ip_;
    int port_;
    int db_;
};


} // namespace tiny

#endif //TINYWORLD_REDIS_H
