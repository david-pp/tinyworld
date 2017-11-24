#include "async.h"

namespace tiny {

static std::atomic<uint64_t> s_total_asynctask = {1};

///////////////////////////////////////////////////////////////////////

AsyncTask::AsyncTask() {
    id_ = s_total_asynctask.fetch_add(1);
    createtime_ = std::chrono::high_resolution_clock::now();
    scheduler_->stat_.construct++;
}

AsyncTask::~AsyncTask() {
    scheduler_->stat_.destroyed++;
}

uint32_t AsyncTask::elapsed_ms() const {
    return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - createtime_).count());
}

bool AsyncTask::isTimeout() const {
    return elapsed_ms() >= timeout_ms_;
}

bool AsyncTask::isNeverTimeout() const {
    return timeout_ms_ == static_cast<uint32_t>(-1);
}

void AsyncTask::call() {
    if (on_call_) on_call_();
}

void AsyncTask::done() {
    if (on_done_) on_done_();
}

void AsyncTask::timeout() {
    if (on_timeout_) on_timeout_();
}

void AsyncTask::cancel() {
    if (on_cancel_) on_cancel_();
}


///////////////////////////////////////////////////////////////////////

AsyncScheduler::AsyncScheduler() {

}

AsyncScheduler::~AsyncScheduler() {

}

void AsyncScheduler::emit(AsyncTaskPtr task) {

}

void AsyncScheduler::run() {
    stat_.queue_wait = queue_wait_.size();
    stat_.queue_wait_by_time = wait_by_time_.size();

    // Check timeout
    auto tmpit = wait_by_time_.begin();
    while (tmpit != wait_by_time_.end()) {
        auto it = tmpit++;

        if (it->second) {
            if (it->second->isTimeout()) {
                triggerTimeout(it->second);
                wait_by_time_.erase(it);
            }
        } else {
            wait_by_time_.erase(it);
        }
    }

    // Trigger all request (TODO: limit maxnum)
    if (1) {
        std::lock_guard<std::mutex> guard(mutex_ready_);

        stat_.queue_ready = queue_ready_.size();

        for (auto &it : queue_ready_)
            triggerCall(it.second);

        queue_ready_.clear();
    }
}

void AsyncScheduler::triggerCall(AsyncTaskPtr task) {

    if (task) {
        queue_wait_.insert(std::make_pair(task->id_, task));
        if (!task->isNeverTimeout())
            wait_by_time_.insert(std::make_pair(task->createtime_, task));

        task->call();

        stat_.call++;
    }

}

void AsyncScheduler::triggerDone(AsyncTaskPtr task) {
    if (task) {
        task->done();

        stat_.done++;

        if (task->parent_) {
            task->parent_->child_done(task);
        }

        queue_wait_.erase(task->id_);
    }
}

void AsyncScheduler::triggerTimeout(AsyncTaskPtr task) {

    if (task) {
        task->timeout();

        stat_.timeout++;

        if (task->parent_) {
            task->parent_->child_timeout(task);
        }

        queue_wait_.erase(task->id_);
    }
}

void AsyncScheduler::triggerCancel(AsyncTaskPtr task) {
    if (task) {
        task->cancel();
        stat_.cancel++;
        queue_wait_.erase(task->id_);
    }
}

} // namespace tiny