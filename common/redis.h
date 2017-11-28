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

#include "redis_cmd.h"

namespace tiny {

class AsyncRedisClient : public AsyncScheduler {
public:
    AsyncRedisClient(const std::string &ip, int port, EventLoop *loop = EventLoop::instance());

    AsyncRedisClient(const std::string &url, EventLoop *loop = EventLoop::instance());

    virtual ~AsyncRedisClient();

    bool isConnected() const;

    bool connect();

    void close();

    void run();

public:
    template<class ReplyT>
    void cmd(const std::vector<std::string> &cmd,
             const std::function<void(RedisCommand<ReplyT> &)> &callback = nullptr) {
        emit(RedisCmd<ReplyT>(cmd, callback));
    }

public:
    void checkConnection();
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
    // Redis Async Context
    redisAsyncContext *context_ = nullptr;

    // Event Loop (libev)
    EventLoop *evloop_ = nullptr;

    // Redis Server's Address(IP, Port, DB)
    std::string ip_;
    int port_ = 0;
    int db_ = 0;
};


} // namespace tiny

#endif //TINYWORLD_REDIS_H
