#ifndef TINYWORLD_ZMQ_SERVER_H
#define TINYWORLD_ZMQ_SERVER_H

#include <zmq.hpp>
#include "message_dispatcher.h"

class ZMQServer
{
public:
    typedef ProtobufMsgDispatcherByName<const std::string&> MsgDispatcher;
    ZMQServer(MsgDispatcher& dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher)
    {

    }

    bool bind(const std::string& address)
    {
        std::cout << "Server listening : " << address << std::endl;
        context_.reset(new zmq::context_t(10));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_ROUTER));
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


class ZMQBroker
{
public:
    ZMQBroker(std::shared_ptr<zmq::context_t> context = NULL)
    {
        if (context == NULL)
            context_.reset(new zmq::context_t(1));
        else
            context_ = context;
    }

    bool init()
    {
        frontend_socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_ROUTER));
        backend_socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));

        frontend_socket_->bind(frontend_address_);
        backend_socket_->bind(backend_address_);

        return true;
    }

    bool fini()
    {
        isrunning_ = false;
    }

    void run(long timeout = -1)
    {
        //  Initialize poll set
        zmq::pollitem_t items [] = {
                { *frontend_socket_.get(), 0, ZMQ_POLLIN, 0 },
                { *backend_socket_.get(),  0, ZMQ_POLLIN, 0 }
        };

        //  Switch messages between sockets
        while (isrunning_) {
            zmq::message_t message;
            int more;               //  Multipart detection

            zmq::poll (&items [0], 2, timeout);

            if (items [0].revents & ZMQ_POLLIN) {
//            s_dump(frontend);
                while (1) {
                    //  Process all parts of the message
                    frontend_socket_->recv(&message);
//                    s_dumpmsg(message);
                    size_t more_size = sizeof (more);
                    frontend_socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
                    backend_socket_->send(message, more? ZMQ_SNDMORE: 0);

                    if (!more)
                        break;      //  Last message part
                }
            }
            if (items [1].revents & ZMQ_POLLIN) {
                while (1) {
                    //  Process all parts of the message
                    backend_socket_->recv(&message);
//                    s_dumpmsg(message);
                    size_t more_size = sizeof (more);
                    backend_socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
                    frontend_socket_->send(message, more? ZMQ_SNDMORE: 0);
                    if (!more)
                        break;      //  Last message part
                }
            }
        }
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> frontend_socket_;
    std::shared_ptr<zmq::socket_t> backend_socket_;

    std::string frontend_address_;
    std::string backend_address_;

    bool isrunning_ = true;
};


#endif //TINYWORLD_ZMQ_SERVER_H
