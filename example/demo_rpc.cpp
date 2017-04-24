#include <iostream>
#include <thread>
#include <chrono>

#include "tinylogger.h"
#include "tinyrpc_client.h"
#include "tinyrpc_server.h"
#include "zmq_client.h"
#include "zmq_server.h"
#include "rpc.pb.h"

DECLARE_MESSAGE_BY_NAME(rpc::GetRequest, "get-req");

DECLARE_MESSAGE_BY_NAME(rpc::GetReply, "get-rep");

//
// Emitter & Dispatcher
//
void demo_emitter() {
    rpc::Request rpc_request;

    //
    // Client Side
    //
    RPCEmitter emitter;
    {
        rpc::GetRequest get;
        get.set_player(1024);
        get.set_name("david");

        emitter.emit<rpc::GetRequest, rpc::GetReply>(get)
                .done([](const rpc::GetReply &reply) {
                    std::cout << "[CLIENT] DONE\t: " << reply.ShortDebugString() << std::endl;
                })
                .timeout([](const rpc::GetRequest &request) {
                    std::cout << "[CLIENT] TIMEOUT\t: " << request.ShortDebugString() << std::endl;
                }, 200)
                .error([](const rpc::GetRequest &request, rpc::ErrorCode errcode) {
                    std::cout << "[CLIENT] ERROR\t: " << rpc::ErrorCode_Name(errcode) << " : "
                              << request.ShortDebugString() << std::endl;
                })
                .pack(rpc_request);

        std::cout << "[CLIENT] EMIT\t: " << rpc_request.ShortDebugString() << std::endl;
    }


//    std::chrono::milliseconds ms(201);
//    std::this_thread::sleep_for(ms);
//    std::cout << "timeouted:" << emitter.checkTimeout() << std::endl;

    // Server Side
    RPCDispatcher dispatcher;
    {
        dispatcher.on<rpc::GetRequest, rpc::GetReply>([](const rpc::GetRequest &req) {
            std::cout << "[SERVER] CALLED\t: " << req.ShortDebugString() << std::endl;

            rpc::GetReply reply;
            reply.set_result("hello!");
            return reply;
        });

        std::cout << "[SERVER] DISPATCH\t: " << rpc_request.ShortDebugString() << std::endl;
        rpc::Reply reply = dispatcher.requested(rpc_request);

        std::cout << "[SERVER] REPLY\t: " << reply.ShortDebugString() << std::endl;
        emitter.replied(reply);
    }
}

//
// RPC Client
//
void demo_client(int id) {
    AsyncRPCClient<ZMQClient> client;
    try {
        client.connect("tcp://localhost:5555");
        while (true) {
            client.poll(1000);
            client.checkTimeout();

            rpc::GetRequest get;
            get.set_player(1024);
            get.set_name("david");
            client.emit<rpc::GetRequest, rpc::GetReply>(get)
                    .done([](const rpc::GetReply &reply) {
                        LOGGER_DEBUG("RPC", "DONE : " << reply.ShortDebugString());
                    })
                    .timeout([](const rpc::GetRequest &request) {
                        LOGGER_DEBUG("RPC", "TIMEOUT : " << request.ShortDebugString());
                    }, 200)
                    .error([](const rpc::GetRequest &request, rpc::ErrorCode errcode) {
                        LOGGER_DEBUG("RPC", "ERROR : " << rpc::ErrorCode_Name(errcode) << " : "
                                                       << request.ShortDebugString());
                    });

            LOGGER_DEBUG("RPC", "EMIT ...");

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

    server.on<rpc::GetRequest, rpc::GetReply>([](const rpc::GetRequest &req) {
        LOGGER_DEBUG("SERVER", req.ShortDebugString());

        rpc::GetReply reply;
        reply.set_result("hello!");
        return reply;
    });

    try {
//        ZMQServer server;
        server.bind("tcp://*:5555");
        while (true) {
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
    if ("emitter" == op) {
        demo_emitter();
    }
    if ("client" == op) {
        demo_client(1);
    } else if ("server" == op) {
        demo_server();
    }
}