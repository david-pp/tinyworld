#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "tinyrpc_client.h"
#include "tinyrpc_server.h"


#include "test_rpc.pb.h"

#include <zmq.h>

#include "message_dispatcher.h"
#include "zmq_client.h"
#include "zmq_server.h"




void test_rpc_client(int id)
{
    AsyncProtobufRPCClient<ZMQClient> client;
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
                        std::cout << "--- DONE ----" << std::endl;
                        std::cout << reply.DebugString() << std::endl;
                    })
                    .timeout([](const rpc::GetRequest &request) {
                        std::cout << "--- TIMEOUT ----" << std::endl;
                        std::cout << request.DebugString() << std::endl;
                    }, 200)
                    .error([](const rpc::GetRequest &request, rpc::ErrorCode errcode) {
                        std::cout << "--- ERROR:" << rpc::ErrorCode_Name(errcode) << " ----" << std::endl;
                        std::cout << request.DebugString() << std::endl;
                    });

            std::cout << "emit rpc..." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void test_rpc_server()
{
    AsyncProtobufRPCServer<ZMQServer> server;

    server.on<rpc::GetRequest, rpc::GetReply>([](const rpc::GetRequest &req) {
        rpc::GetReply reply;
        reply.set_result("fuckyou!");
        std::cout << "RPC ---------------------" << std::endl;
        std::cout << req.DebugString() << std::endl;
        return reply;
    });

    try {
//        ZMQServer server;
        server.bind("tcp://*:5555");
        while (true) {
            server.poll(1000);

            std::cout << "idle ..." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}


void test_client(int id)
{
    ZMQClient::MsgDispatcher::instance()
            .on<rpc::GetReply>([](const rpc::GetReply& msg){
                std::cout << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
            });

    try {
        ZMQClient client;
        client.connect("tcp://localhost:5555");
        while (true) {
            client.poll(1000);

            rpc::GetRequest send;
            send.set_player(id);
            send.set_name("david");
            client.sendMsg(send);

            std::cout << "send.." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void test_server()
{
    ZMQServer server;

    ZMQServer::MsgDispatcher::instance()
            .on<rpc::GetRequest>([&server](const rpc::GetRequest& msg, const std::string& client){
                std::cout << __PRETTY_FUNCTION__ << std::endl;
                std::cout << "client: " << client << std::endl;
                std::cout << msg.DebugString() << std::endl;

                rpc::GetReply reply;
                reply.set_result("cool!!");
                server.sendMsg(client, reply);
            });

    try {
//        ZMQServer server;
        server.bind("tcp://*:5555");
        while (true) {
            server.poll(1000);

            std::cout << "idle ..." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void test_emitter_dispatcher()
{
    rpc::Request rpc_request;
    ProtobufRPCEmitter emitter;
    ProtobufRPCDispatcher dispatcher;

    // client side
    {
        std::cout << "------------- CLIENT ---------------" << std::endl;

        rpc::GetRequest get;
        get.set_player(1024);
        get.set_name("david");
        emitter.emit<rpc::GetRequest, rpc::GetReply>(get)
                .done([](const rpc::GetReply &reply) {
                    std::cout << "--- DONE ----" << std::endl;
                    std::cout << reply.DebugString() << std::endl;
                })
                .timeout([](const rpc::GetRequest &request) {
                    std::cout << "--- TIMEOUT ----" << std::endl;
                    std::cout << request.DebugString() << std::endl;
                }, 200)
                .error([](const rpc::GetRequest &request, rpc::ErrorCode errcode) {
                    std::cout << "--- ERROR:" << rpc::ErrorCode_Name(errcode) << " ----" << std::endl;
                    std::cout << request.DebugString() << std::endl;
                })
                .pack(rpc_request);

        std::cout << rpc_request.DebugString() << std::endl;
    }

//    std::chrono::milliseconds ms(201);
//    std::this_thread::sleep_for(ms);
    std::cout << "timeouted:" << emitter.checkTimeout() << std::endl;

    // server side
    {
        std::cout << "------------- SERVER ---------------" << std::endl;

        dispatcher.on<rpc::GetRequest, rpc::GetReply>([](const rpc::GetRequest &req) {
            rpc::GetReply reply;
            reply.set_result("fuckyou!");
            return reply;
        });

        rpc::Reply reply = dispatcher.requested(rpc_request);
        std::cout << reply.DebugString() << std::endl;

        emitter.replied(reply);
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2 )
    {
        std::cout << "Usage: ... client | server" << std::endl;
        return 0;
    }

    test_emitter_dispatcher();

    std::string op = argv[1];
    if ("client" == op)
    {
        test_client(1);
    }
    else if ("server" == op)
    {
        test_server();
    }
    else if ("rpc_client" == op)
    {
        test_rpc_client(1);
    }
    else if ("rpc_server" == op)
    {
        test_rpc_server();
    }
}