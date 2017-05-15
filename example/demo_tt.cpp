#include <iostream>
#include "tinytable.h"

#include "tinyorm.h"
# include "tinyorm_mysql.h"

#include "tinyrpc_server.h"
#include "zmq_server.h"

#include "player.h"
#include "command.pb.h"

void demo_client() {
    TableClient tc;

//    uint32_t count = 0;
    bool done = true;
    try {
        tc.connect("tcp://localhost:5555");
        while (done) {
            tc.poll(1000);
            tc.checkTimeout();
//
//            tc.get<Player>(9, 10000)
//                    .done([](const Player &p) {
//                        LOG_DEBUG("tinytable", "get-done");
//                        std::cout << p << std::endl;
//                    })
//                    .timeout([](uint32_t key) {
//                        LOG_DEBUG("tinytable", "get-timeout");
//                    })
//                    .nonexist([](uint32_t key) {
//                        LOG_DEBUG("tinytable", "get-nonexist");
//                    });
//
//            Player p;
//            p.init();
//            p.id = 101;
//            p.age = ++count;
//            tc.set<Player>(p, 10000)
//                    .done([](const Player &p) {
//                        LOG_DEBUG("tinytable", "set okay");
//                    });
//
//            tc.del<Player>(101, 10000)
//                    .done([](uint32_t key) {
//                        LOG_DEBUG("tinytable", "del okay:%u", key);
//                    });

            tc.load<Player>(1000)
                    .done([](const std::vector<Player> &players) {
                        LOG_DEBUG("tinytable", "------------");
                        for (auto &p : players)
                            LOG_DEBUG("tinytable", "%s", p.name.c_str());
                    });
////
            std::chrono::milliseconds ms(1000);
            std::this_thread::sleep_for(ms);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void demo_server() {
    AsyncRPCServer<ZMQAsyncServer> server;

    server.on<tt::GetRequest, tt::GetReply>([](const tt::GetRequest &req) {
        LOGGER_DEBUG("SERVER-GET", req.ShortDebugString());

        tt::GetReply reply;
        reply.set_type(req.type());

        Player p;
        deserialize(p.id, req.key());
//        p.init();

        TinyORM db;
        if (db.select(p)) {
            reply.set_value(serialize(p));
            reply.set_retcode(0);
        } else
            reply.set_retcode(1);
        return reply;
    }).on<tt::Set, tt::SetReply>([](const tt::Set &req) {
        LOGGER_DEBUG("SERVER-SET", req.ShortDebugString());

        tt::SetReply rep;
        rep.set_type(req.type());

        Player p;
        if (deserialize(p, req.value())) {
            TinyORM db;
            if (db.replace(p)) {
                rep.set_retcode(0);
            } else
                rep.set_retcode(1);
        }
        return rep;
    }).on<tt::Del, tt::DelReply>([](const tt::Del &req) {
        LOGGER_DEBUG("SERVER-DEL", req.ShortDebugString());

        Player::TableKey key;
        deserialize(key, req.key());

        Player p;
        p.id = key;
        TinyMySqlORM db;
        db.del(p);

        tt::DelReply rep;
        rep.set_retcode(0);
        return rep;
    }).on<tt::LoadRequest, tt::LoadReply>([](const tt::LoadRequest &request) {
        TinyORM db;
        std::vector<Player> players;
        db.loadFromDB<Player>([&players](std::shared_ptr<Player> record) {
            players.push_back(*record.get());
            std::cout << record->name << std::endl;
        }, NULL);

        tt::LoadReply reply;
        reply.set_type(request.type());
        reply.set_values(serialize(players));

        return reply;
    });

    bool done = true;
    try {
//        ZMQServer server;
        server.bind("tcp://*:5555");
        while (done) {
            server.poll();
//            LOGGER_DEBUG("SERVER", "idle ...")
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ... client | server" << std::endl;
        return 0;
    }

    MySqlConnectionPool::instance().connect("mysql://david:123456@127.0.0.1/tinyworld");

    std::string op = argv[1];

    if ("client" == op) {
        demo_client();
    } else if ("server" == op) {
        demo_server();
    }
}