#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "zmq_client.h"
#include "zmq_server.h"

#include "test_msg.pb.h"

std::shared_ptr<zmq::context_t> g_context;

void test_client(int id)
{
    ZMQClient::MsgDispatcher::instance()
            .on<Cmd::LoginReply>([](const Cmd::LoginReply& msg){
                std::cout << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;
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
            client.sendMsg(send);

            std::cout << "send.." << std::endl;

            std::chrono::milliseconds ms(1000);
            std::this_thread::sleep_for(ms);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void test_server()
{
    ZMQAsyncServer server;

    ZMQAsyncServer::MsgDispatcher::instance()
            .on<Cmd::LoginRequest, Cmd::LoginReply>([&server](const Cmd::LoginRequest& msg, const std::string& client){
                LOG_DEBUG("ZMQAsyncServer", "%s", msg.DebugString().c_str());
                Cmd::LoginReply reply;
                reply.set_info("ZMQAsyncServer");
//                server.sendMsg(client, reply);
                return reply;
            });

    try {
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
    ZMQWorker worker(ZMQWorker::MsgDispatcher::instance(), g_context);

    ZMQWorker::MsgDispatcher::instance()
            .on<Cmd::LoginRequest, Cmd::LoginReply>([&worker](const Cmd::LoginRequest& msg){
                std::cout << __PRETTY_FUNCTION__ << std::endl;
                std::cout << msg.DebugString() << std::endl;

                Cmd::LoginReply reply;
                reply.set_info("worker reply!!");
                return reply;
            });

    try {
        worker.connect("tcp://localhost:6666");
//        worker.bind("tcp://*:5555");
//        worker.connect("inproc://workers");

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
    ZMQBroker broker(g_context);

    try {
        broker.bind("tcp://*:5555", "tcp://*:6666");
//        broker.bind("tcp://*:5555", "inproc://workers");

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
    g_context.reset(new zmq::context_t(2));

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
        std::thread t(test_worker);
        test_broker();
        t.join();
    }
}