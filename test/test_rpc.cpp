#include <iostream>
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

    ProtobufRPCEmitter rpc;

    std::string bin;

    rpc::GetRequest get;
    get.set_player(1024);
    get.set_name("david");
    rpc.emit<rpc::GetRequest, rpc::GetReply>(get)
            .done([](const rpc::GetReply& reply){

            })
            .timeout([](const rpc::GetRequest& request){

            })
            .pack2String(bin);

}