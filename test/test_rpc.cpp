#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "rpc_client.h"
#include "rpc_server.h"


#include "test_rpc.pb.h"


class AsyncRPCClient
{
public:
    template <typename RequestT>
    void emit(const RequestT& request)
    {
    }

private:
    ProtobufRPCEmitter emitter_;
};

class AsyncRPCServer
{
public:
private:
};


int main(int argc, const char* argv[])
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