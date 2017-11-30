#include <iostream>
#include <thread>
#include "async.h"
#include "eventloop.h"
#include "redis.h"
#include "tinylogger.h"

uint32_t randint(uint64_t start, uint64_t end) {
    uint32_t value = std::rand();
    return start + value * (end - start) / RAND_MAX;
}

using namespace tiny;
using namespace tiny::redis;

AsyncRedisClient redis_cli("127.0.0.1", 6379);

void hmget(AsyncRedisClient &redis_cli, const std::string &key,
           const std::function<void(std::map<std::string, std::string>)> &callback) {

    redis_cli.cmd<std::vector<std::string>>({"HGETALL", key}, [callback](RedisCommand<std::vector<std::string>> &c) {
        if (c.ok()) {
            std::map<std::string, std::string> kvs;
            for (uint64_t i = 0; i < c.reply().size() && i + 1 < c.reply().size(); i += 2) {
                kvs[c.reply()[i]] = c.reply()[i + 1];
            }
            callback(kvs);
        }
    });
}

void test_async() {
    AsyncScheduler async;

    std::thread async_thread([&async]() {
        bool value = true;
        while (value) {
            async.run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

//    async.emit(std::make_shared<AsyncTask>([&async]() {
//                                               std::cout << "[task] call - start" << std::endl;
//                                               std::this_thread::sleep_for(std::chrono::seconds(randint(0, 5)));
//                                               std::cout << "[task] call - done" << std::endl;
//                                           },
//                                           []() {
//                                               std::cout << "[task] done ..." << std::endl;
//                                           }));

    async_thread.join();
}

void test_eventloop() {
    EventLoop loop;
    loop.onIdle([]() {
                std::cout << "Idle1.." << std::endl;
            })
            .onIdle([]() {
                std::cout << "Idle2.." << std::endl;
            })
            .onTimer([]() {
                std::cout << "1s.." << std::endl;
            }, 1)
            .onTimer([]() {
                std::cout << "0.5s.." << std::endl;
            }, 0.5);

    loop.run();
}

void demo_simple() {

    redis_cli.cmd<std::string>({"SET", "myname", "David++"});
    redis_cli.cmd<std::string>({"SET", "myage", "31"});

    redis_cli.cmd<std::string>({"GET", "myname"}, [](RedisCommand<std::string> &c) {
        if (c.ok()) {
            LOGGER_INFO("simple1", "Hello, " << c.reply());
        } else {
            LOGGER_ERROR("simple1", c.lastError() << " - " << c.status());
        }
    });

    redis_cli.emit(
            RedisCmd<std::string>({"GET", "myname"}, [](RedisCommand<std::string> &c) {
                if (c.ok()) {
                    LOGGER_INFO("simple2", "Hello, " << c.reply());
                } else {
                    LOGGER_ERROR("simple2", c.lastError() << " - " << c.status());
                }
            }));
}

void demo_parallel() {

    struct User {
        std::string name;
        int age = 0;
    };

    auto user = std::make_shared<User>();

    redis_cli.emit(AsyncTask::P({
                                        RedisCmd<std::string>({"GET", "myname"},
                                                              [user](RedisCommand<std::string> &c) {
                                                                  user->name = c.reply();
                                                                  LOGGER_INFO("parallel", "GET name = " << user->name);
                                                              }),
                                        RedisCmd<std::string>({"GET", "myage"},
                                                              [user](RedisCommand<std::string> &c) {
                                                                  user->age = std::atoi(c.reply().c_str());
                                                                  LOGGER_INFO("parallel", "GET age = " << user->age);
                                                              }),
                                },
                                [user]() {
                                    LOGGER_INFO("parallel", "Done - User:" << user->name << "\t" << user->age);
                                }));
}

void demo_series() {

    auto values = std::make_shared<std::string>();

    redis_cli.emit(AsyncTask::S({
                                        RedisCmd<std::string>({"GET", "myname"},
                                                              [values](RedisCommand<std::string> &c) {
                                                                  values->append(c.reply());
                                                                  values->append(" ");
                                                                  LOGGER_INFO("series", "GET name = " << c.reply());
                                                              }),
                                        RedisCmd<std::string>({"GET", "myage"},
                                                              [values](RedisCommand<std::string> &c) {
                                                                  values->append(c.reply());
                                                                  values->append("-");
                                                                  LOGGER_INFO("series", "GET age = " << c.reply());
                                                              }),
                                },
                                [values]() {
                                    LOGGER_INFO("series", "Done - " << *values);
                                }));
}


struct RedisGets : public AsyncTask, public std::map<std::string, std::string> {
public:
    RedisGets(const std::vector<std::string> &keys) : keys_(keys) {}

    virtual ~RedisGets() {
//            std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

protected:
    void on_emit(AsyncScheduler *scheduler) override {
        AsyncTask::on_emit(scheduler);

        std::vector<AsyncTaskPtr> tasks;
        for (auto &key : keys_) {
            tasks.push_back(RedisCmd<std::string>(
                    {"GET", key},
                    [this, key](RedisCommand<std::string> &c) {
                        LOGGER_INFO("userdef", "GET " << key << "=" << c.reply());
                        insert(std::make_pair(key, c.reply()));
                    }));
        }

        emit(AsyncTask::P(tasks, std::bind(&AsyncTask::emit_done, this)));
    }

private:
    std::vector<std::string> keys_;
};

void demo_userdefine() {

    redis_cli.emit(AsyncTask::T<RedisGets, std::vector<std::string>>(
            {"myname", "myage"},
            [](RedisGets &gets) {
                LOGGER_INFO("userdef", "Done - " << gets["myname"] << "\t" << gets["myage"]);
            }));
}

void demo_compound() {

    redis_cli.emit(AsyncTask::S({
                                        AsyncTask::T<RedisGets, std::vector<std::string>>(
                                                {"myname"},
                                                [](RedisGets &gets) {
                                                    LOGGER_INFO("compound-1", gets["myname"]);
                                                }),
                                        AsyncTask::T<RedisGets, std::vector<std::string>>(
                                                {"myage"},
                                                [](RedisGets &gets) {
                                                    LOGGER_INFO("compound-2", gets["myage"]);
                                                }),
                                },
                                []() {
                                    LOGGER_INFO("compound-3", "series done");

                                    redis_cli.cmd<std::string>({"GET", "myname"}, [](RedisCommand<std::string> &c) {
                                        LOGGER_INFO("compound-4", "Hello, " << c.reply());
                                    });
                                }));
}

void demo_hash() {

    redis_cli.exec(HMSET("user:1000",
                              {"name", "charid", "age"},
                              {"David++", std::to_string(1000), std::to_string(31)}));

    redis_cli.exec(HMSET("user:1000", {
            {"name",   "David++"},
            {"charid", std::to_string(1000)},
            {"age",    std::to_string(31)},
    }));

    redis_cli.exec(HMGET("user:1000", {"name", "charid", "age"}, [](KeyValueHashMap &user) {
        LOGGER_INFO("hash", "name   = " << user["name"]);
        LOGGER_INFO("hash", "charid = " << user["charid"]);
        LOGGER_INFO("hash", "age    = " << user["age"]);
    }));
}

void demo_key() {


    redis_cli.exec(DEL("user:1000"));

    redis_cli.exec(INCR("ID", [](uint64_t id) {
        LOGGER_INFO("key", "ID=" << id);
    }));

    redis_cli.exec(GET("ID", [](const std::string &id) {
        LOGGER_INFO("key", "ID=" << id);
    }));
}

void demo_list() {
    using namespace tiny::redis;

    redis_cli.exec(DEL("userlist"));
    redis_cli.exec(LPUSH("userlist", {"David++", "Lica", "Jessica"}, [](RedisCommand<uint32_t> &c) {
        redis_cli.exec(LRANGE("userlist", 0, -1, [](const StringVector &values) {
            for (auto &v : values) {
                LOGGER_INFO("list", v);
            }
        }));
    }));
}

int main(int argc, const char *argv[]) {
    std::srand(std::time(0));

    if (argc < 2) {
        std::cout << "Usage:" << argv[0]
                  << " simple | hash | key | list" << std::endl;
        return 1;
    }

    std::string op = argv[1];

    redis_cli.connect();

    EventLoop::instance()->onTimer([op]() {
        if ("simple" == op)
            demo_simple();
        else if ("parallel" == op)
            demo_parallel();
        else if ("series" == op)
            demo_series();
        else if ("user" == op)
            demo_userdefine();
        else if ("compound" == op)
            demo_compound();
        else if ("hash" == op)
            demo_hash();
        else if ("list" == op)
            demo_list();
        else if ("key" == op)
            demo_key();
    }, 2, 1);

    EventLoop::instance()->onTimer([]() {
        LOGGER_INFO("demo", redis_cli.statString());
    }, 1);

    EventLoop::instance()->run();
}