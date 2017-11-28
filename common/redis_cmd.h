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
#include "tinylogger.h"

namespace tiny {

class AsyncRedisClient;

//
// Redis Command Async Task.
//
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

    // Redis command
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

} // namespace tiny

#endif //TINYWORLD_REDIS_COMMAND_H
