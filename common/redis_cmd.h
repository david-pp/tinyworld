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

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <hiredis/async.h>
#include "async.h"
#include "eventloop.h"

namespace tiny {

class AsyncRedisClient;

typedef std::vector<std::string> StringVector;
typedef std::set<std::string> StringSet;
typedef std::unordered_set<std::string> StringHashSet;
typedef std::unordered_map<std::string, std::string> KeyValueHashMap;

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
    ReplyT &reply();

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

//
// Helper Function: Create a Redis Command with Callback
//
template<typename ReplyT>
inline AsyncTaskPtr
RedisCmd(std::vector<std::string> cmd, const std::function<void(RedisCommand<ReplyT> &)> &callback = nullptr) {
    if (cmd.empty())
        return nullptr;

    return std::make_shared<RedisCommand<ReplyT>>(nullptr, cmd, callback);
}

//
// Redis Commands. Encapsulated in namespace redis.
//
namespace redis {

//
// Strings (https://redis.io/commands#string)
//
inline AsyncTaskPtr
SET(const std::string &key, const std::string &value,
    const std::function<void(RedisCommand<std::string> &)> &callback = nullptr) {
    return RedisCmd<std::string>({"SET", key, value}, callback);
}

inline AsyncTaskPtr
GET(const std::string &key,
    const std::function<void(const std::string &value)> &callback) {
    return RedisCmd<std::string>({"GET", key}, [callback](RedisCommand<std::string> &c) {
        callback(c.reply());
    });
}

inline AsyncTaskPtr
INCR(const std::string &key,
     const std::function<void(uint64_t value)> &callback) {
    return RedisCmd<uint64_t>({"INCR", key}, [callback](RedisCommand<uint64_t> &c) {
        callback(c.reply());
    });
}


//
// Keys (https://redis.io/commands#generic)
//

inline AsyncTaskPtr
DEL(const std::string &key, const std::function<void(RedisCommand<uint32_t> &)> &callback = nullptr) {
    return RedisCmd<uint32_t>({"DEL", key}, callback);
}

inline AsyncTaskPtr
DEL(const std::vector<std::string> &keys,
    const std::function<void(RedisCommand<uint32_t> &)> &callback = nullptr) {
    std::vector<std::string> cmd = {"DEL"};
    cmd.insert(cmd.end(), keys.begin(), keys.end());
    return RedisCmd<uint32_t>(cmd, callback);
}

inline AsyncTaskPtr
EXPIRE(const std::string &key,
       uint32_t seconds,
       const std::function<void(RedisCommand<uint32_t> &)> &callback = nullptr) {
    return RedisCmd<uint32_t>({"EXPIRE", key, std::to_string(seconds)}, callback);
}

inline AsyncTaskPtr
EXPIREAT(const std::string &key,
         uint32_t timepoint,
         const std::function<void(RedisCommand<uint32_t> &)> &callback = nullptr) {
    return RedisCmd<uint32_t>({"EXPIREAT", key, std::to_string(timepoint)}, callback);
}

//
// HMSET
//
inline AsyncTaskPtr
HMSET(const std::string &key,
      const std::vector<std::string> &fields,
      const std::vector<std::string> &values,
      const std::function<void(RedisCommand<std::string> &)> &callback = nullptr) {

    if (key.empty() || fields.empty() || values.empty())
        return nullptr;

    if (fields.size() != values.size())
        return nullptr;

    std::vector<std::string> cmd{"HMSET", key};
    for (size_t i = 0; i < fields.size(); ++i) {
        cmd.push_back(fields[i]);
        cmd.push_back(values[i]);
    }

    return RedisCmd<std::string>(cmd, callback);
}

inline AsyncTaskPtr
HMSET(const std::string &key,
      const std::unordered_map<std::string, std::string> &kvs,
      const std::function<void(RedisCommand<std::string> &)> &callback = nullptr) {
    std::vector<std::string> fields;
    std::vector<std::string> values;

    for (auto &kv : kvs) {
        fields.push_back(kv.first);
        values.push_back(kv.second);
    }

    return HMSET(key, fields, values, callback);
}

//
// HMGET
//
inline AsyncTaskPtr
HMGET(const std::string &key,
      const std::vector<std::string> &fields,
      const std::function<void(KeyValueHashMap &)> &callback) {

    std::vector<std::string> cmd{"HMGET", key};
    cmd.insert(cmd.end(), fields.begin(), fields.end());

    return RedisCmd<StringVector>(cmd, [fields, callback](RedisCommand<StringVector> &c) {
        if (c.ok() && c.reply().size() == fields.size()) {
            KeyValueHashMap kvs;
            for (uint64_t i = 0; i < c.reply().size(); ++i) {
                kvs.insert(std::make_pair(fields[i], c.reply()[i]));
            }
            callback(kvs);
        }
    });
}

//
// LPUSH
//
inline AsyncTaskPtr
LPUSH(const std::string &key,
      const std::vector<std::string> &values,
      const std::function<void(RedisCommand<uint32_t> &)> &callback = nullptr) {

    std::vector<std::string> cmd{"LPUSH", key};
    cmd.insert(cmd.end(), values.begin(), values.end());
    return RedisCmd<uint32_t>(cmd, callback);
}

//
// LRANGE
//
inline AsyncTaskPtr
LRANGE(const std::string &key,
       int start = 0, int stop = -1,
       const std::function<void(const StringVector &)> &callback = nullptr) {

    return RedisCmd<StringVector>({"LRANGE", key, std::to_string(start), std::to_string(stop)},
                                  [callback](RedisCommand<StringVector> &c) {
                                      if (c.ok()) {
                                          if (callback)
                                              callback(c.reply());
                                      }
                                  });
}

//
// ZADD
//
inline AsyncTaskPtr
ZADD(const std::string &key,
     const uint64_t score,
     const std::string &value,
     const std::function<void(uint32_t)> &callback = nullptr) {

    return RedisCmd<uint32_t>({"ZADD", key, std::to_string(score), value},
                              [callback](RedisCommand<uint32_t> &c) {
                                  if (c.ok()) {
                                      if (callback)
                                          callback(c.reply());
                                  }
                              });
}

inline AsyncTaskPtr
ZADD(const std::string &key,
     const std::multimap<uint64_t, std::string> &score_value,
     const std::function<void(uint32_t)> &callback = nullptr) {

    std::vector<std::string> cmd{"ZADD", key};
    for (auto &kv : score_value) {
        cmd.push_back(std::to_string(kv.first));
        cmd.push_back(kv.second);
    }

    return RedisCmd<uint32_t>(cmd,
                              [callback](RedisCommand<uint32_t> &c) {
                                  if (c.ok()) {
                                      if (callback)
                                          callback(c.reply());
                                  }
                              });
}

//
// ZRANGE
//
inline AsyncTaskPtr
ZRANGE(const std::string &key,
       const int64_t start, const int64_t stop,
       const std::function<void(const StringVector &)> &callback) {

    return RedisCmd<StringVector>({"ZRANGE", key, std::to_string(start), std::to_string(stop)},
                                  [callback](RedisCommand<StringVector> &c) {
                                      if (c.ok()) {
                                          if (callback)
                                              callback(c.reply());
                                      }
                                  });
}

inline AsyncTaskPtr
ZRANGE_WITHSCORES(const std::string &key,
                  const int64_t start, const int64_t stop,
                  const std::function<void(const std::multimap<int64_t, std::string> &)> &callback) {

    return RedisCmd<StringVector>({"ZRANGE", key, std::to_string(start), std::to_string(stop), "WITHSCORES"},
                                  [callback](RedisCommand<StringVector> &c) {
                                      if (c.ok()) {
                                          std::multimap<int64_t, std::string> kvs;
                                          for (uint64_t i = 0;
                                               i < c.reply().size() && i + 1 < c.reply().size(); i += 2) {
                                              kvs.insert(std::make_pair(std::atol(c.reply()[i + 1].c_str()),
                                                                        c.reply()[i]));
                                          }
                                          if (callback) callback(kvs);
                                      }
                                  });
}


} // namespace redis
} // namespace tiny

#endif //TINYWORLD_REDIS_COMMAND_H
