#include <sstream>
#include "async.h"
#include "tinylogger.h"

namespace tiny {

static std::atomic<uint64_t> s_total_asynctask = {1};

///////////////////////////////////////////////////////////////////////

AsyncTask::AsyncTask() {
    createtime_ = std::chrono::high_resolution_clock::now();
}

AsyncTask::~AsyncTask() {
    if (scheduler_) scheduler_->stat_.destroyed++;
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

void AsyncTask::emit(AsyncTaskPtr task) {
    if (scheduler_)
        scheduler_->emit(task);
}

void AsyncTask::emit_done() {
    if (scheduler_)
        scheduler_->triggerDone(shared_from_this(), nullptr);
}

bool AsyncTask::addChildByOrder(AsyncTaskPtr task) {
    if (task) {
        children_by_order_.push_back(task);
        task->parent_ = this;
        return true;
    }

    return false;
}

AsyncTaskPtr AsyncTask::on_call(const Callback &callback) {
    on_call_ = callback;
    return shared_from_this();
}

AsyncTaskPtr AsyncTask::on_done(const Callback &callback) {
    on_done_ = callback;
    return shared_from_this();
}

AsyncTaskPtr AsyncTask::on_timeout(const Callback &callback) {
    on_timeout_ = callback;
    return shared_from_this();
}

void AsyncTask::call() {
    if (on_call_) on_call_();
}

void AsyncTask::done(void *data) {
    if (on_done_) on_done_();
}

void AsyncTask::timeout() {
    if (on_timeout_) on_timeout_();
}

void AsyncTask::cancel() {
    if (on_cancel_) on_cancel_();

    for (auto it : children_) {
        if (it.second)
            scheduler_->triggerCancel(it.second);
    }

    children_.clear();
}

AsyncTaskPtr AsyncTask::P(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done) {
    auto task = std::make_shared<ParallelTask>();
    if (task) {
        task->on_done_ = done;
        for (auto child : children)
            task->addChildByOrder(child);
    }
    return task;
}

AsyncTaskPtr AsyncTask::S(const std::vector<AsyncTaskPtr> &children, const std::function<void()> &done) {
    auto task = std::make_shared<SerialsTask>();
    if (task) {
        task->on_done_ = done;
        for (auto child : children)
            task->addChildByOrder(child);
    }
    return task;
}

void AsyncTask::on_emit(AsyncScheduler *scheduler) {
    if (!scheduler) return;

    id_ = s_total_asynctask.fetch_add(1);
    scheduler_ = scheduler;
    scheduler_->stat_.construct++;

    for (auto child : children_by_order_) {
        if (child) {
            child->on_emit(scheduler_);
            children_.insert(std::make_pair(child->id_, child));
        }
    }
}


///////////////////////////////////////////////////////////////////////

SerialsTask::~SerialsTask() {
}

void SerialsTask::call() {
    AsyncTask::call();
    triggerFirstCall();
}

void SerialsTask::child_done(AsyncTaskPtr child) {
    if (!child) return;

    auto first = children_.begin();
    if (first != children_.end() && first->second->id_ == child->id_) {
        children_.erase(child->id_);
        triggerFirstCall();
    }

    if (children_.empty())
        scheduler_->triggerDone(shared_from_this(), nullptr);
}

void SerialsTask::child_timeout(AsyncTaskPtr child) {
    // this will timeout next tick
    timeout_ms_ = 0;
}

void SerialsTask::triggerFirstCall() {
    auto first = children_.begin();
    if (first != children_.end()) {
        scheduler_->triggerCall(first->second);
    }
}

AsyncTaskPtr SerialsTask::clone() {
    std::vector<AsyncTaskPtr> children_clone_;
    for (auto child : children_by_order_) {
        auto child_clone = child->clone();
        if (child_clone)
            children_clone_.push_back(child_clone);
    }

    return AsyncTask::S(children_clone_, on_done_);
}

///////////////////////////////////////////////////////////////////////

ParallelTask::~ParallelTask() {
}

void ParallelTask::call() {
    AsyncTask::call();

    for (auto it : children_by_order_) {
        scheduler_->triggerCall(it);
    }
}

void ParallelTask::child_done(AsyncTaskPtr child) {
    if (!child) return;

    auto it = children_.find(child->id_);
    if (it != children_.end())
        children_.erase(it);

    if (children_.empty())
        scheduler_->triggerDone(shared_from_this(), nullptr);
}

void ParallelTask::child_timeout(AsyncTaskPtr child) {
    // this will timeout next tick
    timeout_ms_ = 0;
}

AsyncTaskPtr ParallelTask::clone() {
    std::vector<AsyncTaskPtr> children_clone_;
    for (auto child : children_by_order_) {
        auto child_clone = child->clone();
        if (child_clone)
            children_clone_.push_back(child_clone);
    }

    return AsyncTask::P(children_clone_, on_done_);
}

///////////////////////////////////////////////////////////////////////

AsyncScheduler::AsyncScheduler() {

}

AsyncScheduler::~AsyncScheduler() {

}

void AsyncScheduler::emit(AsyncTaskPtr task) {
    if (!task) return;

    task->on_emit(this);

    std::lock_guard<std::mutex> guard(mutex_ready_);
    queue_ready_.push_back(task);
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

    // Trigger all request
    if (true) {

        std::deque<AsyncTaskPtr> queue_ready;
        {
            std::lock_guard<std::mutex> guard(mutex_ready_);
            queue_ready = queue_ready_;
            stat_.queue_ready = queue_ready_.size();
            queue_ready_.clear();
        }

        for (auto task : queue_ready)
            triggerCall(task);
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

bool AsyncScheduler::triggerDone(uint64_t id, void *data) {
    auto it = queue_wait_.find(id);
    if (it != queue_wait_.end()) {
        triggerDone(it->second, data);
        return true;
    }
    return false;
}

void AsyncScheduler::triggerDone(AsyncTaskPtr task, void *data) {
    if (task) {
        task->done(data);

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

std::string AsyncScheduler::statString() {
    std::ostringstream oss;
    oss << "Queue:" << stat_.queue_wait << "," << stat_.queue_wait_by_time
        << " Task:" << stat_.construct << "," << stat_.destroyed
        << " Task:" << stat_.call << "," << stat_.done << "," << stat_.timeout << "," << stat_.cancel;
    return oss.str();
}


} // namespace tiny