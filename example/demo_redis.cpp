#include <iostream>
#include <thread>
#include "async.h"
#include "eventloop.h"
#include "redis.h"

uint32_t randint(uint64_t start, uint64_t end) {
    uint32_t value = std::rand();
    return start + value * (end - start) / RAND_MAX;
}

using namespace tiny;

#if 0
struct GetUser {
    GetUser(User *u) {}

    void request() {
        cmd("HMGE user:xxx");
    }

    void done() {
        user->id = 0;
    }

    User *user;
};


void test_1() {
    RedisClient redis;
    redis->connect();

    rdx.command<string>({"GET", "occupation"}, [](RedisCommand <string> &c) {
        if (c.ok()) {
            cout << "Hello, async " << c.reply() << endl;
        } else {
            cerr << "RedisCommand has error code " << c.status() << endl;
        }
    });

    struct Data {
        User a;
        int b;
    };

    auto data = std::make_shared<Data>();
    rdx.parallel([data]() {
                     // done
                     data->a + data->b;
                 },
                 GetUser(&data->a, 100),

                 {"GET user:xxx", [&data]() {
                     data->a = 10;
                 }},
                 {"GET user:xxx", [data]() {
                     data->b = 20;
                 }});


    cmd("get user")
            .done([](Reply) {

            })
            .timeout([]() {

            })
            .error([]() {
            });

    cmd("get user", [](RedisCommand &c) {
        if (c.error()) {

        }

        if (c.timeout) {

        }
    });
}

#endif

AsyncRedisClient redis("127.0.0.1", 6379);


struct Reply {
    std::string string;
    int integer;
};

struct Cmd {
    std::vector<std::string> cmd;
    std::function<void(const Reply &)> cb;

    std::function<void()> cb_timeout;

    Cmd timeout(std::function<void()> cb_time) {
        cb_timeout = cb_time;
        return *this;
    }
};

Cmd parallel(const std::vector<Cmd> &cmds, std::function<void()> cb) {
    Cmd c;
    return c;
}

Cmd makecmd(std::vector<std::string> cmd, std::function<void(const Reply &)> cb) {
    Cmd c;
    c.cmd = cmd;
    c.cb = cb;
    return c;
}

void hmget(AsyncRedisClient &redis, const std::string &key,
           const std::function<void(std::map<std::string, std::string>)> &callback) {

    redis.cmd<std::vector<std::string>>({"HGETALL", key}, [callback](RedisCommand<std::vector<std::string>> &c) {
        if (c.ok()) {
            std::map<std::string, std::string> kvs;
            for (uint64_t i = 0; i < c.reply().size() && i + 1 < c.reply().size(); i += 2) {
                kvs[c.reply()[i]] = c.reply()[i + 1];
            }
            callback(kvs);
        }
    });
}

void test_redis_1() {

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
    redis.connect();

    redis.cmd<std::string>({"SET", "myname", "David++"});
    redis.cmd<std::string>({"SET", "myage", "31"});

//    EventLoop::instance()->onTimer([]() {
    redis.cmd<std::string>({"GET", "myname"}, [](RedisCommand<std::string> &c) {
        if (c.ok()) {
            std::cout << "[simple] Hello, " << c.reply() << std::endl;
        } else {
            std::cerr << "[simple] Error: " << c.lastError() << " - " << c.status() << std::endl;
        }
    });

    redis.emit(
            RedisCmd<std::string>({"GET", "myname"}, [](RedisCommand<std::string> &c) {
                if (c.ok()) {
                    std::cout << "[simple] Hello, " << c.reply() << std::endl;
                } else {
                    std::cerr << "[simple] Error: " << c.lastError() << " - " << c.status() << std::endl;
                }
            }));

//    }, 2);
}

void demo_parallel() {

    struct User {
        std::string name;
        int age = 0;
    };

    auto user = std::make_shared<User>();

    redis.emit(AsyncTask::P({
                                    RedisCmd<std::string>({"GET", "myname"},
                                                          [user](RedisCommand<std::string> &c) {
                                                              user->name = c.reply();
                                                              std::cout << "P:GET name = "
                                                                        << user->name << std::endl;
                                                          }),
                                    RedisCmd<std::string>({"GET", "myage"},
                                                          [user](RedisCommand<std::string> &c) {
                                                              user->age = std::atoi(c.reply().c_str());
                                                              std::cout << "P:GET age = " << user->age << std::endl;
                                                          }),
                            },
                            [user]() {
                                std::cout << "P:Done User: " << user->name << "\t" << user->age << std::endl;
                            }));
}

void demo_series() {
    struct User {
        std::string name;
        int age = 0;
    };

    auto user = std::make_shared<User>();

    redis.emit(AsyncTask::S({
                                    RedisCmd<std::string>({"GET", "myname"},
                                                          [user](RedisCommand<std::string> &c) {
                                                              user->name = c.reply();
                                                              std::cout << "S:GET name = "
                                                                        << user->name << std::endl;
                                                          }),
                                    RedisCmd<std::string>({"GET", "myage"},
                                                          [user](RedisCommand<std::string> &c) {
                                                              user->age = std::atoi(c.reply().c_str());
                                                              std::cout << "S:GET age = " << user->age << std::endl;
                                                          }),
                            },
                            [user]() {
                                std::cout << "S:Done User: " << user->name << "\t" << user->age << std::endl;
                            }));
}


void demo_userdefine() {

    struct RedisGet : public AsyncTask, public std::map<std::string, std::string> {
    public:
        RedisGet(const std::vector<std::string> &keys) : keys_(keys) {}

        void call() override {

            std::vector<AsyncTaskPtr> tasks;
            for (auto &key : keys_) {
                tasks.push_back(RedisCmd<std::string>(
                        {"GET", key},
                        [this, key](RedisCommand<std::string> &c) {
                            std::cout << c.reply() << std::endl;
                            insert(std::make_pair(key, c.reply()));
                        }));
            }

            emit(AsyncTask::P(tasks, std::bind(&AsyncTask::emit_done, this)));
        }

    private:
        std::vector<std::string> keys_;
    };

    redis.emit(AsyncTask::T<RedisGet, std::vector<std::string>>(
            {"myname", "myage"},
            [](std::shared_ptr<RedisGet> gets) {
                std::cout << "S:Done User:" << (*gets)["myname"] << "\t" << (*gets)["myage"] << std::endl;
            }));
}


int main() {
    std::srand(std::time(0));

    demo_simple();
    demo_parallel();
    demo_series();
    demo_userdefine();
//    test_async();
//    test_redis_1();

    EventLoop::instance()->onTimer([]() {
        std::cout << redis.statString() << std::endl;
    }, 1);

    EventLoop::instance()->run();
}