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


#ifndef TINYWORLD_ASYNC_H
#define TINYWORLD_ASYNC_H

#include <map>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

namespace tiny {

class AsyncTask;

class SerialsTask;

class ParallelTask;

class AsyncScheduler;

typedef std::shared_ptr<AsyncTask> AsyncTaskPtr;

class AsyncTask : public std::enable_shared_from_this<AsyncTask> {
public:
    friend class AsyncScheduler;

    friend class SerialsTask;

    friend class ParallelTask;

    static AsyncTaskPtr P(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done);

    static AsyncTaskPtr S(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done);

    template<typename TaskT>
    static AsyncTaskPtr T(const std::function<void(std::shared_ptr<TaskT>)> &done) {
        auto task = std::make_shared<TaskT>();
        task->on_done_ = [done, task]() {
            done(task);
        };
        return task;
    }

public:
    AsyncTask();

    AsyncTask(AsyncScheduler *scheduler) : AsyncTask() {
        scheduler_ = scheduler;
    }

    AsyncTask(const std::function<void()> &on_call, const std::function<void()> &on_done)
            : AsyncTask() {
        on_call_ = on_call;
        on_done_ = on_done;
    }

    uint32_t elapsed_ms() const;

    bool isTimeout() const;

    bool isNeverTimeout() const;

    uint64_t id() const { return id_; }

    void emit(AsyncTaskPtr task);

public:
    virtual ~AsyncTask();

    virtual void call();

    virtual void done(void *data);

    virtual void timeout();

    virtual void cancel();

    virtual void child_done(AsyncTaskPtr child) {}

    virtual void child_timeout(AsyncTaskPtr child) {}

protected:
    void on_emit(AsyncScheduler *scheduler);

protected:
    // Identifier
    uint64_t id_ = 0;

    // Create Time
    std::chrono::time_point<std::chrono::high_resolution_clock> createtime_;

    // For Timeout
    uint32_t timeout_ms_ = static_cast<uint32_t>(-1);

    // Task Hierarchy
    AsyncTask *parent_ = nullptr;
    std::unordered_map<uint64_t, AsyncTaskPtr> children_;

    // Scheduler
    AsyncScheduler *scheduler_ = nullptr;

    std::function<void()> on_call_;
    std::function<void()> on_done_;
    std::function<void()> on_timeout_;
    std::function<void()> on_cancel_;
};

class SerialsTask : public AsyncTask {
public:
    virtual ~SerialsTask();

    void call() override;

    void done(void *data) override;

    void timeout() override;

    void cancel() override;

    void child_done(AsyncTaskPtr child) override;

    void child_timeout(AsyncTaskPtr child) override;

protected:
    void triggerFirstCall();
};

class ParallelTask : public AsyncTask {
public:
    virtual ~ParallelTask();

    void call() override;

    void done(void *data) override;

    void timeout() override;

    void cancel() override;

    void child_done(AsyncTaskPtr child) override;

    void child_timeout(AsyncTaskPtr child) override;
};


///////////////////////////////////////////////////
//
// Scheduler:
//
//   thread1  thread2  thread3
//      |        |        |   - call
//      V        V        V
//     RRRRRRRRRRRRRRRRRRRRR  - ready queue
//               |
//  +-----------thread:run -------------------+
//  |            |  - request                 |
//  |            V                            |
//  |    WWWWWWWWWWWWWWWWWW  - waiting queue  |
//  |       | -reply  | -timeout              |
//  |       v         v                       |
//  |   triggerCall  triggerTimeout           |
//  |                                         |
//  +-----------------------------------------+
//
///////////////////////////////////////////////////
class AsyncScheduler {
public:
    friend class AsyncTask;

    AsyncScheduler();

    virtual ~AsyncScheduler();

    void emit(AsyncTaskPtr task);

    void run();

    struct Stat {
        std::atomic<uint64_t> construct = {0};
        std::atomic<uint64_t> destroyed = {0};

        std::atomic<uint64_t> call = {0};
        std::atomic<uint64_t> done = {0};
        std::atomic<uint64_t> timeout = {0};
        std::atomic<uint64_t> cancel = {0};

        std::atomic<uint64_t> queue_ready = {0};
        std::atomic<uint64_t> queue_wait = {0};
        std::atomic<uint64_t> queue_wait_by_time = {0};
    };

    Stat &stat();

    std::string statString();

public:
    void triggerCall(AsyncTaskPtr task);

    bool triggerDone(uint64_t id, void *data);

    void triggerDone(AsyncTaskPtr task, void *data);

    void triggerTimeout(AsyncTaskPtr task);

    void triggerCancel(AsyncTaskPtr task);

protected:
    // Ready Queue
    std::mutex mutex_ready_;
    std::unordered_map<uint64_t, AsyncTaskPtr> queue_ready_;

    // Waiting Queue
    std::unordered_map<uint64_t, AsyncTaskPtr> queue_wait_;

    // Waiting Ordered by Create Time
    std::multimap<std::chrono::time_point<std::chrono::high_resolution_clock>,
            AsyncTaskPtr> wait_by_time_;

    // Statistics
    Stat stat_;
};

} // namespace tiny

#endif //TINYWORLD_ASYNC_H
