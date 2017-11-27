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

    redis.cmd<std::string>({"GET", "occupation"}, [](RedisCommand<std::string> &c) {
        if (c.ok()) {
            std::cout << "Hello, async " << c.reply() << std::endl;
        } else {
            std::cerr << "RedisCommand has error code " << c.status() << std::endl;
        }
    });

    redis.cmd<std::vector<std::string>>({"HGETALL", "user"}, [](RedisCommand<std::vector<std::string>> &c) {
        if (c.ok()) {
            for (auto &v :c.reply()) {
                std::cout << v << std::endl;
            }
        }
    });

    hmget(redis, "user", [](std::map<std::string, std::string> kvs) {
        for (auto &it : kvs) {
            std::cout << it.first << ":" << it.second << std::endl;
        }
    });



//    struct User : public AsyncTask {
//        std::string name;
//        int sex = 0;
//
//        void call() override {
//            emit(AsyncTask::P({RedisCmd<std::string>({"HMGET", "this:xx", "name"},
//                                                     [this](RedisCommand<std::string> &c) {
//                                                         this->name = c.reply();
//                                                     }),
//                               RedisCmd<int>({"HMGET", "this:xx", "sex"},
//                                             [this](RedisCommand<int> &c) {
//                                                 this->sex = c.reply();
//                                             }),
//                              },
//                              [this]() {
//                                  this->done(nullptr);
//                              }));
//        }
//    };
//
//    auto user = std::make_shared<User>();
//
//    auto task = AsyncTask::P({
//                                     RedisCmd<std::string>({"HMGET", "user:xx", "name"},
//                                                           [user](RedisCommand<std::string> &c) {
//                                                               user->name = c.reply();
//                                                           }),
//                                     RedisCmd<int>({"HMGET", "user:xx", "sex"},
//                                                   [user](RedisCommand<int> &c) {
//                                                       user->sex = c.reply();
//                                                   }),
//                                     AsyncTask::T<User>([](std::shared_ptr<User> u) {
//                                     }),
//
//                                     AsyncTask::S({}, []() {}),
//                             },
//                             [user]() {
//                                 std::cout << user->name << user->name << std::endl;
//                             });
//
//
//    parallel({
//                     {{"GET", "NAME"}, [user](const Reply &c) {
//                         user->name = c.string;
//                     }},
//                     {{"GET", "SEX"},  [user](const Reply &c) {
//                         user->sex = c.integer;
//                     }},
////                     task<User>(),
//             },
//             [user]() {
//                 std::cout << user->name << user->sex << std::endl;
//             });

//    parallel({
//                     makecmd({"GET", "NAME"}, [user](const Reply &c) {
//                         user->name = c.string;
//                     }),
//                     makecmd({"GET", "SEX"}, [user](const Reply &c) {
//                         user->sex = c.integer;
//                     }),
//                     parallel({
//                                      makecmd({"GET", "X"}, [user](const Reply &c) {
//                                          user->name = c.string;
//                                      }),
//                                      makecmd({"GET", "Y"}, [user](const Reply &c) {
//                                          user->sex = c.integer;
//                                      }),
//                              },
//                              []() {
//                                  // ...
//                              }),
//             },
//             [user]() {
//                 std::cout << user->name << user->sex << std::endl;
//             });
//
//
//    series([]() {
//
//    }, []() {
//
//    });
//
//    a([]() {
//        b([]() {
//            c([]() {
//
//            })
//        })
//    });


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

    async.emit(std::make_shared<AsyncTask>([&async]() {
                                               std::cout << "[task] call - start" << std::endl;
                                               std::this_thread::sleep_for(std::chrono::seconds(randint(0, 5)));
                                               std::cout << "[task] call - done" << std::endl;
                                           },
                                           []() {
                                               std::cout << "[task] done ..." << std::endl;
                                           }));

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

int main() {
    std::srand(std::time(0));
//    test_async();
    test_redis_1();

    EventLoop::instance()->run();
}