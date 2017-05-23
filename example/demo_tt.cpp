#include <iostream>
#include "tinytable_client.h"
#include "tinytable_server.h"

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
            tc.get<Player>(9, 10000)
                    .done([](const Player &p) {
                        LOGGER_DEBUG("tinytable", "get-done : " << p.name);
                    })
                    .timeout([](uint32_t key) {
                        LOG_DEBUG("tinytable", "get-timeout");
                    })
                    .nonexist([](uint32_t key) {
                        LOG_DEBUG("tinytable", "get-nonexist");
                    });
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

//            tc.load<Player>(1000)
//                    .done([](const std::vector<Player> &players) {
//                        LOG_DEBUG("tinytable", "------------");
//                        for (auto &p : players)
//                            LOG_DEBUG("tinytable", "%s", p.name.c_str());
//                    });
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

//    TinyTableFactory::instance().dbTable<Player>();
//    TinyTableFactory::instance().memTable<Player>();
    TinyTableFactory::instance().cacheTable<Player>();
    TinyTableFactory::instance().connectDB("mysql://david:123456@127.0.0.1/tinyworld");

    TableServer server;
    bool done = true;
    try {
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

    std::string op = argv[1];
    if ("client" == op) {
        demo_client();
    } else if ("server" == op) {
        demo_server();
    }
}


//    TinyTable<Player> tt_player(&MySqlConnectionPool::instance());
//
//    server.on<tt::Get, tt::GetReply>([&tt_player](const tt::Get &req) {
//        tt::GetReply reply;
//        tt_player.processGet(req, reply);
//        return reply;
//
//    }).on<tt::Set, tt::SetReply>([&tt_player](const tt::Set &req) {
//        tt::SetReply reply;
//        tt_player.proceeSet(req, reply);
//        return reply;
//
//    }).on<tt::Del, tt::DelReply>([&tt_player](const tt::Del &req) {
//        tt::DelReply reply;
//        tt_player.proceeDel(req, reply);
//        return reply;
//
//    }).on<tt::LoadRequest, tt::LoadReply>([](const tt::LoadRequest &request) {
//        TinyORM db;
//        std::vector<Player> players;
//        db.loadFromDB<Player>([&players](std::shared_ptr<Player> record) {
//            players.push_back(*record.get());
//            std::cout << record->name << std::endl;
//        }, NULL);
//
//        tt::LoadReply reply;
//        reply.set_type(request.type());
//        reply.set_values(serialize(players));
//
//        return reply;
//    });