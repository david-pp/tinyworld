#include "redis.h"
#include "tinylogger.h"
#include "url.h"
#include "hiredis/adapters/libev.h"

namespace tiny {

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
        LOGGER_INFO("redis", "redis://" << ip_ << ":" << port_
                                        << " async connected success");
    } else {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_
                                         << " async connected failed:" << context_->errstr);
        context_ = NULL;
    }
}

void AsyncRedisClient::onDisconnected(bool success) {
    context_ = NULL;
    LOGGER_INFO("redis", "redis://" << ip_ << ":" << port_
                                    << " async disconnected");
}

bool AsyncRedisClient::connect() {
    if (!evloop_) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_
                                         << " Connection error: event loop is not ready");
        return false;
    }

    context_ = redisAsyncConnect(ip_.c_str(), port_);
    if (context_ == NULL) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_
                                         << " Connection error: can't allocate redis context");
        return false;
    }

    if (context_->err) {
        LOGGER_ERROR("redis", "redis://" << ip_ << ":" << port_
                                         << " Connection error: " << context_->errstr);
        close();
        return false;
    }

    context_->data = this;

    redisLibevAttach(evloop_->evLoop(), context_);
    redisAsyncSetConnectCallback(context_, connectCallback);
    redisAsyncSetDisconnectCallback(context_, disconnectCallback);
    return true;
}

bool AsyncRedisClient::isConnected() const {
    return context_ && context_->err == 0;
}

void AsyncRedisClient::checkConnection() {
    if (!isConnected())
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

} // namespace tiny