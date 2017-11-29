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
#include <queue>
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

//
// Async Task Base Class
//
class AsyncTask : public std::enable_shared_from_this<AsyncTask> {
public:
    friend class AsyncScheduler;
    friend class SerialsTask;
    friend class ParallelTask;

    typedef std::function<void()> Callback;

    //
    // Helper Function: Create a parallel async task with callback.
    //
    static AsyncTaskPtr P(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done = nullptr);

    //
    // Helper Function: Create a series async task with callback.
    //
    static AsyncTaskPtr S(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done = nullptr);

    //
    // Helper Function: Create a user-defined async task with callback.
    //
    template<typename TaskT, typename... ArgTypes>
    static AsyncTaskPtr T(ArgTypes... args, const std::function<void(TaskT &task)> &done = nullptr) {
        auto task = std::make_shared<TaskT>(args...);
        auto taskptr = task.get();
        task->on_done_ = [done, taskptr]() { done(*taskptr); };
        return task;
    }

public:
    AsyncTask();

    AsyncTask(AsyncScheduler *scheduler) : AsyncTask() {
        scheduler_ = scheduler;
    }

    uint64_t id() const { return id_; }

    uint32_t elapsed_ms() const;

    bool isTimeout() const;
    bool isNeverTimeout() const;

    //
    // Emit a Task with scheduler_. For Example:
    //
    //    struct MyTask : public AsyncTask {
    //        std::string name;
    //        int sex = 0;
    //
    //        void call() override {
    //            emit(AsyncTask::P({...},
    //                              std::bind(&AsyncTask::emit_done, this)));
    //        }
    //    };
    //
    void emit(AsyncTaskPtr task);
    void emit_done();

    //
    // Add a sub-task
    //
    bool addChildByOrder(AsyncTaskPtr task);

    //
    // Set Callbacks
    //
    AsyncTaskPtr on_call(const Callback &callback);
    AsyncTaskPtr on_done(const Callback &callback);
    AsyncTaskPtr on_timeout(const Callback &callback);

public:
    virtual ~AsyncTask();

    //
    // Not necessary except if you want to use the same task twice:
    //
    //  auto task = AsyncTask::P({...}, [](){});
    //  scheduler.emit(task);          // good
    //  scheduler.emit(task);          // bad.(the same task can't be scheduled twice)
    //  scheduler.emit(task->clone()); // good
    //
    virtual AsyncTaskPtr clone() { return nullptr; }

    // Call in the event loop.
    virtual void call();

    // Done when the task is over.
    virtual void done(void *data);

    // Timeout
    virtual void timeout();

    // useless...
    virtual void cancel();

protected:
    virtual void on_emit(AsyncScheduler *scheduler);

    virtual void child_done(AsyncTaskPtr child) {}

    virtual void child_timeout(AsyncTaskPtr child) {}

protected:
    // Identifier
    uint64_t id_ = 0;

    // Create Time
    std::chrono::time_point<std::chrono::high_resolution_clock> createtime_;

    // For Timeout
    uint32_t timeout_ms_ = static_cast<uint32_t>(-1);

    // Task Hierarchy
    AsyncTask *parent_ = nullptr;
    std::map<uint64_t, AsyncTaskPtr> children_;
    std::deque<AsyncTaskPtr> children_by_order_;

    // Scheduler
    AsyncScheduler *scheduler_ = nullptr;

    // Callbacks
    std::function<void()> on_call_;
    std::function<void()> on_done_;
    std::function<void()> on_timeout_;
    std::function<void()> on_cancel_;
};

//
// Tasks will be called one by one.
//
//  task1.done() -> task2.done() -> ... -> series.done()
//
class SerialsTask : public AsyncTask {
public:
    virtual ~SerialsTask();

    AsyncTaskPtr clone() override;

    void call() override;

protected:
    void child_done(AsyncTaskPtr child) override;

    void child_timeout(AsyncTaskPtr child) override;

    void triggerFirstCall();
};

//
// Tasks will be called at the sametime.
//
//        +-> task1.done() -> -+
//        |                    |
//     ---|-> task2.done() -> -+- -> parallel.done()
//        |                    |
//        +-> task3.done() -> -+
//
class ParallelTask : public AsyncTask {
public:
    virtual ~ParallelTask();

    AsyncTaskPtr clone() override;

    void call() override;

protected:
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

public:
    //
    // Statistics of the Scheduler
    //
    struct Stat {
        std::atomic<uint64_t> construct = {0};          // number of task emitted
        std::atomic<uint64_t> destroyed = {0};          // number of task destruct

        std::atomic<uint64_t> call = {0};               // number of call
        std::atomic<uint64_t> done = {0};               // number of done
        std::atomic<uint64_t> timeout = {0};            // number of timeout
        std::atomic<uint64_t> cancel = {0};             // number of cancel

        std::atomic<uint64_t> queue_ready = {0};        // size of ready queue
        std::atomic<uint64_t> queue_wait = {0};         // size of wait queue
        std::atomic<uint64_t> queue_wait_by_time = {0}; // size of wait queue sorted by time
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
    std::deque<AsyncTaskPtr> queue_ready_;

    // Waiting Queue
    std::unordered_map<uint64_t, AsyncTaskPtr> queue_wait_;

    // Waiting Queue Ordered by Create Time
    std::multimap<std::chrono::time_point<std::chrono::high_resolution_clock>,
            AsyncTaskPtr> wait_by_time_;

    // Statistics
    Stat stat_;
};

} // namespace tiny

#endif //TINYWORLD_ASYNC_H
