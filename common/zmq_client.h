#ifndef TINYWORLD_ZMQ_CLIENT_H
#define TINYWORLD_ZMQ_CLIENT_H

#include <zmq.hpp>
#include "message_dispatcher.h"

class ZMQClient
{
public:
    typedef ProtobufMsgDispatcherByName<> MsgDispatcher;

    ZMQClient(MsgDispatcher& dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher)
    {
    }

    bool setID(const std::string& id)
    {
        if (id.length())
        {
            socket_->setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
        }
        return true;
    }

    bool connect(const std::string& address)
    {
        std::cout << "Connecting to  server..." << std::endl;
        context_.reset(new zmq::context_t(1));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));
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
                zmq::message_t empty;
                zmq::message_t request;

                socket_->recv(&empty);
                socket_->recv(&request);

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

        zmq::message_t empty;
        zmq::message_t msg(msgbuf.data(), msgbuf.size());
        socket_->send(empty, ZMQ_SNDMORE);
        socket_->send(msg);
    }

protected:
    void on_recv(zmq::message_t& request)
    {
        MessageHeader* header = (MessageHeader*)request.data();
        if (request.size() >= sizeof(MessageHeader))
        {
            std::cout << header->dumphex() << std::endl;

            if (header->msgsize() == request.size())
            {
                try {
                    msg_dispatcher_.dispatch(header);
                    return;
                }
                catch (std::exception& err)
                {
                    std::cout << "ERROR:" << err.what() << std::endl;
                }
            }
        }

        std::cout << "ERROR: " << request.size() << std::endl;
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    ProtobufMsgDispatcherByName<>& msg_dispatcher_;
};


#endif //TINYWORLD_ZMQ_CLIENT_H
