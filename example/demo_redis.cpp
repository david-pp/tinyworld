#include <iostream>
#include "eventloop.h"

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

    rdx.command<string>({"GET", "occupation"}, [](Command <string> &c) {
        if (c.ok()) {
            cout << "Hello, async " << c.reply() << endl;
        } else {
            cerr << "Command has error code " << c.status() << endl;
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

    cmd("get user", [](Command &c) {
        if (c.error()) {

        }

        if (c.timeout) {

        }
    });
}

#endif

int main() {
    using namespace tiny;

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