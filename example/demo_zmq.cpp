#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "zmq_client.h"
#include "zmq_server.h"

#include "command.pb.h"

//
// Async Client
//
void demo_client(int id) {
    ZMQClient::MsgDispatcher::instance()
            .on<Cmd::LoginReply>([](const Cmd::LoginReply &msg) {
                LOGGER_DEBUG("CLIENT", msg.ShortDebugString());
            });

    try {
        ZMQClient client;
        client.connect("tcp://localhost:5555");
        int count = 0;
        while (true) {
            client.poll(1000);

            Cmd::LoginRequest send;
            send.set_id(count++);
            send.set_type(20);
            send.set_name("david");
            client.send(send);

            LOGGER_DEBUG("CLIENT", "send ...");

            std::chrono::milliseconds ms(1000);
            std::this_thread::sleep_for(ms);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

//
// Async Server
//
void demo_server() {
    ZMQAsyncServer server;

    ZMQAsyncServer::MsgDispatcher::instance()
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &msg, const std::string &client) {
                LOGGER_DEBUG("ZMQAsyncServer", msg.ShortDebugString());
                Cmd::LoginReply reply;
                reply.set_info("ZMQAsyncServer");
                return reply;
            });

    try {
        server.bind("tcp://*:5555");
        while (true) {
            server.poll(1000);

            LOGGER_DEBUG("ZMQAsyncServer", "idle ...");
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}


static std::shared_ptr<zmq::context_t> g_context;

//
// Sync/Worker Server
//
void demo_worker(bool isworker) {
    ZMQWorker worker(ZMQWorker::MsgDispatcher::instance(), g_context);

    ZMQWorker::MsgDispatcher::instance()
            .on<Cmd::LoginRequest, Cmd::LoginReply>([](const Cmd::LoginRequest &msg) {
                LOGGER_DEBUG("WORKER", msg.ShortDebugString());

                Cmd::LoginReply reply;
                reply.set_info("worker reply!!");
                return reply;
            });

    try {
        if (isworker)
            worker.connect("tcp://localhost:6666");
            //        worker.connect("inproc://workers");
        else
            worker.bind("tcp://*:5555");

        while (true) {
            worker.poll(1000);

            LOGGER_DEBUG("WORKER", "idle ...");
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void demo_broker() {
    ZMQBroker broker(g_context);

    try {
        broker.bind("tcp://*:5555", "tcp://*:6666");
//        broker.bind("tcp://*:5555", "inproc://workers");

        while (true) {
            broker.poll(1000);
            LOGGER_DEBUG("BROKER", "idle ...");
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}


int main(int argc, const char *argv[]) {
//    g_context.reset(new zmq::context_t(2));

    if (argc < 2) {
        std::cout << "Usage: ... client | server" << std::endl;
        return 0;
    }

    std::string op = argv[1];
    if ("client" == op) {
        demo_client(1);
    } else if ("server" == op) {
        demo_server();
    } else if ("sync_server" == op) {
        demo_worker(false);
    } else if ("broker" == op) {
        demo_broker();
    } else if ("worker" == op) {
        demo_worker(true);
    }
}