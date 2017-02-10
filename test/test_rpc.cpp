#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "rpc_client.h"
#include "rpc_server.h"


#include "test_rpc.pb.h"

#include <zmq.h>
#include <zmq.hpp>
#include "message_dispatcher.h"
#include "../dataserver/command.pb.h"

class ZMQClient
{
public:
    typedef ProtobufMsgDispatcherByName<> MsgDispatcher;

    ZMQClient(MsgDispatcher& dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher)
    {
        context_.reset(new zmq::context_t(1));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));

        std::ostringstream id;
        id << "aclient";
        socket_->setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());
    }

    bool connect(const std::string& address)
    {
        std::cout << "Connecting to  server..." << std::endl;
        socket_->connect(address);
        return true;
    }

    bool poll(long timeout = -1)
    {
        zmq::pollitem_t items[] = { { *socket_.get(), 0, ZMQ_POLLIN, 0 } };

        int rc = zmq_poll (&items[0], 1, timeout);
        if (-1 != rc)
        {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t request;
                socket_->recv (&request);

                this->on_recv(request);
            }
        }

        return true;
    }

    template <typename MSG>
    void sendMsg(MSG& proto)
    {
        std::string msgbuf;
        ProtobufMsgHandlerBase::packByName(msgbuf, proto);
        zmq::message_t msg(msgbuf.data(), msgbuf.size());
        socket_->send(msg);
    }

protected:
    void on_recv(zmq::message_t& request)
    {
        MessageHeader* header = (MessageHeader*)request.data();

        std::cout << header->dumphex() << std::endl;

        if (header->msgsize() == request.size())
        {
            msg_dispatcher_.dispatch(header);
        }
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    ProtobufMsgDispatcherByName<>& msg_dispatcher_;
};

class ZMQServer
{
public:
    typedef ProtobufMsgDispatcherByName<const std::string&> MsgDispatcher;
    ZMQServer(MsgDispatcher& dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher)
    {
        context_.reset(new zmq::context_t(10));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_ROUTER));
    }

    bool bind(const std::string& address)
    {
        std::cout << "Server listening : " << address << std::endl;
        socket_->bind(address);
        return true;
    }

    bool poll(long timeout = -1)
    {
        zmq::pollitem_t items[] = { { *socket_.get(), 0, ZMQ_POLLIN, 0 } };

        int rc = zmq_poll (&items[0], 1, timeout);
        if (-1 != rc)
        {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t idmsg;
                socket_->recv(&idmsg);
                std::string id((char*)idmsg.data(), idmsg.size());

                zmq::message_t request;
                socket_->recv (&request);

                on_recv(id, request);
            }
        }

        return true;
    }


    template <typename MSG>
    void sendMsg(const std::string& id, MSG& proto)
    {
        std::string msgbuf;
        ProtobufMsgHandlerBase::packByName(msgbuf, proto);

        zmq::message_t idmsg(id.data(), id.size());
        zmq::message_t reply(msgbuf.data(), msgbuf.size());

        socket_->send(idmsg, ZMQ_SNDMORE);
        socket_->send(reply);
    }

protected:
    void on_recv(std::string& client, zmq::message_t& request)
    {
        MessageHeader* header = (MessageHeader*)request.data();

        std::cout << header->dumphex() << std::endl;

        if (header->msgsize() == request.size())
        {
            msg_dispatcher_.dispatch(header, client);
        }
    }
private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    ProtobufMsgDispatcherByName<const std::string&>& msg_dispatcher_;
};


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