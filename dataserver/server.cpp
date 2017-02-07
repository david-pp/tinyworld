#include <iostream>
#include <string>
#include <zmq.hpp>
#include "message_dispatcher.h"

ProtobufMsgDispatcherByName dispatcher;

//  Prepare our context and sockets
zmq::context_t* context = NULL;
zmq::socket_t*  clients_socket_ = NULL;


void on_recv(std::string& id, zmq::message_t& request)
{
    MessageHeader* header = (MessageHeader*)request.data();

    std::cout << header->dumphex() << std::endl;

    if (header->msgsize() == request.size())
    {
        dispatcher.dispatch(header, id);
    }
}

template <typename MSG>
void sendMsg(std::string& id, MSG& proto)
{
    std::string msgbuf;
    ProtobufMsgDispatcherByName::pack(msgbuf, &proto);

    zmq::message_t idmsg(id.data(), id.size());
    zmq::message_t reply(msgbuf.data(), msgbuf.size());

    clients_socket_->send(idmsg, ZMQ_SNDMORE);
    clients_socket_->send(reply);
}

void server()
{
    context = new zmq::context_t(10);
    clients_socket_ = new zmq::socket_t(*context, ZMQ_ROUTER);

    clients_socket_->bind ("tcp://*:5555");

    zmq::pollitem_t items[] = { { *clients_socket_, 0, ZMQ_POLLIN, 0 } };
    while (true)
    {
        int rc = zmq_poll (&items[0], 1, 1000);
        if (-1 != rc)
        {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t idmsg;
                clients_socket_->recv(&idmsg);
                std::string id((char*)idmsg.data(), idmsg.size());

                zmq::message_t request;
                clients_socket_->recv (&request);

                on_recv(id, request);
            }
        }

        std::cout << "timer..." << std::endl;
    }
}


#include "command.pb.h"

void onGet(Cmd::Get* msg, std::string client_id)
{
    std::cout << msg->DebugString() << std::endl;

    Cmd::GetReply send;
    send.set_id_(msg->id_());
    send.set_key(msg->key());
    send.set_type(msg->type());
    sendMsg(client_id, send);
}

void onSet(Cmd::Set* msg, std::string client_id)
{
    std::cout << msg->DebugString() << std::endl;
}

int main(int argc, const char* argv[])
{
    dispatcher.bind<Cmd::Get, std::string>(onGet);
    dispatcher.bind<Cmd::Set, std::string>(onSet);

    server();
}
