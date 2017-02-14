#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "zmq_client.h"
#include "zmq_server.h"

#include "test_rpc.pb.h"

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

void test_worker()
{
    ZMQWorker worker;
    worker.on<rpc::GetRequest, rpc::GetReply>([](const rpc::GetRequest &req) {
        rpc::GetReply reply;
        reply.set_result("fuckyou!");
        std::cout << "RPC ---------------------" << std::endl;
        std::cout << req.DebugString() << std::endl;
        return reply;
    });

    try {
//        worker.connect("tcp://localhost:6666");
        worker.bind("tcp://*:5555");

        while (true) {
            worker.poll(1000);

            std::cout << "idle ..." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void test_broker()
{
    ZMQBroker broker;

    try {
//        worker.connect("tcp://localhost:6666");
        broker.bind("tcp://*:5555", "tcp://*:6666");

        while (true) {
            broker.poll(1000);

            std::cout << "idle ..." << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}


int main(int argc, const char* argv[])
{
    if (argc < 2 )
    {
        std::cout << "Usage: ... client | server" << std::endl;
        return 0;
    }

    std::string op = argv[1];
    if ("client" == op)
    {
        test_client(1);
    }
    else if ("server" == op)
    {
        test_server();
    }
    else if ("worker" == op)
    {
        test_worker();
    }
    else if ("broker" == op)
    {
        test_broker();
    }
}