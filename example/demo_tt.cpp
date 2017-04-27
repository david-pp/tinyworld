#include <iostream>
#include "tinytable.h"

#include "tinyrpc_server.h"
#include "zmq_server.h"

struct Player {
    uint32_t id;
    std::string name;
};

void demo_client() {
    TableClient tc;

    bool done = true;
    try {
        tc.connect("tcp://localhost:5555");
        while (done) {
            tc.poll(1000);
            tc.checkTimeout();

            tc.get<Player>(20u, 200)
                    .done([](const Player &p) {
                        LOG_DEBUG("tinytable", "done");
                    })
                    .timeout([](uint32_t key){
                        LOG_DEBUG("tinytable", "timeout");
                    });

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
        LOGGER_DEBUG("SERVER", req.ShortDebugString());

        tt::GetReply reply;
        reply.set_type(req.type());
        reply.set_value("ttttttt");
        return reply;
    });

    bool done = true;
    try {
//        ZMQServer server;
        server.bind("tcp://*:5555");
        while (done) {
            server.poll(1000);
            LOGGER_DEBUG("SERVER", "idle ...")
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