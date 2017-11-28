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

#ifndef TINYWORLD_REDIS_COMMAND_H
#define TINYWORLD_REDIS_COMMAND_H

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

namespace tiny {

class AsyncRedisClient;

typedef std::vector<std::string> StringVector;
typedef std::set<std::string> StringSet;
typedef std::unordered_set<std::string> StringHashSet;

//
// Redis Command Status Code
//
enum RedisCmdStatus : int {
    no,                      // No reply yet
    okay,                    // Successful reply of the expected type
    nil,                     // Got a nil reply
    error,                   // Could not send to server
    send_error,              // Could not send to server
    wrone_type,              // Got reply, but it was not the expected type
    timeout,                 // No reply, timed out
};

//
// Redis Command Async Task. ReplyT can be the following:
//
//  - redisReply *
//  - std::string
//  - char *
//  - int/long long int
//  - uint16_t/uint32_t/uint64_t
//  - nullptr_t
//  - StringVector/std::vector<std::string>
//  - StringSet/std::set<std::string>
//  - StringHashSet/std::unordered_set<std::string>
//
template<typename ReplyT>
class RedisCommand : public AsyncTask {
public:
    friend class AsyncRedisClient;

    RedisCommand(AsyncRedisClient *redis, const std::vector<std::string> &cmd,
                 const std::function<void(RedisCommand<ReplyT> &)> &callback)
            : redis_(redis), cmd_(cmd), callback_(callback) {}

    virtual ~RedisCommand();

    // Clone the Command object
    AsyncTaskPtr clone() override;

    // Submit the Command to Redis Server
    void call() override;

    // Receive the Reply from Redis Server
    void done(void *data) override;

    // Timeout
    void timeout() override;

    // scheduler type is AsyncRedisClient
    void on_emit(AsyncScheduler *scheduler) override;

public:
    // Returns the reply status of this command.
    RedisCmdStatus status() const { return reply_status_; }

    // Last error message.
    std::string lastError() const { return last_error_; }

    // Returns true if this command got a successful reply.
    bool ok() const { return reply_status_ == RedisCmdStatus::okay; }

    // Returns the reply value, if the reply was successful (ok() == true).
    ReplyT reply();

    // Returns the command string represented by this object.
    std::string cmd() const;

protected:
    // Handles a new reply from the server
    void processReply(redisReply *r);

    // Invoke a user callback from the reply object. This method is specialized
    // for each ReplyT of RedisCommand.
    void parseReplyObject();

    // If needed, free the redisReply
    void freeReply();

    // Directly invoke the user callback if it exists
    void invoke() {
        if (callback_)
            callback_(*this);
    }

    bool checkErrorReply();

    bool checkNilReply();

    bool isExpectedReply(int type);

    bool isExpectedReply(int typeA, int typeB);

protected:
    RedisCommand(const RedisCommand &) = delete;
    RedisCommand &operator=(const RedisCommand &) = delete;

    // Async redis client
    AsyncRedisClient *redis_ = nullptr;

    // Redis command
    std::vector<std::string> cmd_;

    // The last server reply
    redisReply *reply_obj_ = nullptr;

    // User callback
    const std::function<void(RedisCommand<ReplyT> &)> callback_;

    // Place to store the reply value and status.
    ReplyT reply_val_;
    RedisCmdStatus reply_status_ = RedisCmdStatus::no;
    std::string last_error_;
};

template<typename ReplyT>
inline AsyncTaskPtr
RedisCmd(std::vector<std::string> cmd, const std::function<void(RedisCommand<ReplyT> &)> callback = nullptr) {
    if (cmd.empty())
        return nullptr;

    return std::make_shared<RedisCommand<ReplyT>>(nullptr, cmd, callback);
}

} // namespace tiny

#endif //TINYWORLD_REDIS_COMMAND_H
