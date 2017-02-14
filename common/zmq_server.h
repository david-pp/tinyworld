#ifndef TINYWORLD_ZMQ_SERVER_H
#define TINYWORLD_ZMQ_SERVER_H

#include <thread>
#include <zmq.hpp>
#include "message_dispatcher.h"

#include <iostream>
#include <string>
#include <iomanip>
static void s_dumpmsg(zmq::message_t & message)
{
    //  Dump the message as text or binary
    int size = message.size();
    std::string data(static_cast<char*>(message.data()), size);

    bool is_text = true;

    int char_nbr;
    unsigned char byte;
    for (char_nbr = 0; char_nbr < size; char_nbr++) {
        byte = data [char_nbr];
        if (byte < 32 || byte > 127)
            is_text = false;
    }
    std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";
    for (char_nbr = 0; char_nbr < size; char_nbr++) {
        if (is_text)
            std::cout << (char)data [char_nbr];
        else
            std::cout << std::setfill('0') << std::setw(2)
                      << std::hex << (unsigned int) data [char_nbr];
    }
    std::cout << std::endl;
}


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

    bool bind(const std::string& frontend, const std::string& backend)
    {
        frontend_address_ = frontend;
        backend_address_ = backend;

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

    void poll(long timeout = -1)
    {
        //  Initialize poll set
        zmq::pollitem_t items [] = {
                { *frontend_socket_.get(), 0, ZMQ_POLLIN, 0 },
                { *backend_socket_.get(),  0, ZMQ_POLLIN, 0 }
        };

        //  Switch messages between sockets
        if (isrunning_) {
            zmq::message_t message;
            int more;               //  Multipart detection

            zmq::poll (&items [0], 2, timeout);

            if (items [0].revents & ZMQ_POLLIN) {
                while (1) {
                    //  Process all parts of the message
                    frontend_socket_->recv(&message);
                    s_dumpmsg(message);
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
                    s_dumpmsg(message);
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

#include "rpc_server.h"

class ZMQWorker
{
public:
    ZMQWorker(std::shared_ptr<zmq::context_t> context = NULL)
    {
        if (context == NULL)
            context_.reset(new zmq::context_t(1));
        else
            context_ = context;

        msg_dispatcher_instance_
                .on<rpc::Request>(std::bind(&ZMQWorker::doRPCRequest,
                                            this, std::placeholders::_1));
    }

    bool connect(const std::string& address)
    {

        broker_address_ = address;
//        thread_.reset(new std::thread(&ZMQWorker::run, this, -1));
        std::cout << "Connecting to  Broker..." << std::endl;
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_REP));
        socket_->connect(broker_address_);

        return true;
    }

    bool bind(const std::string& address)
    {
        if (!context_) return false;
        std::cout << "Server listening : " << address << std::endl;
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_REP));
        socket_->bind(address);

        return true;
    }

    void close()
    {
        isrunning_ = false;
//        thread_->join();

//        std::cout << "thread  is over.." << std::endl;
    }

    bool poll(long timeout = -1)
    {
        zmq::pollitem_t items[] = { { *socket_.get(), 0, ZMQ_POLLIN, 0 } };

        int rc = zmq_poll(&items[0], 1, timeout);
        if (-1 != rc) {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t request;
                socket_->recv(&request);

                s_dumpmsg(request);

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
        zmq::message_t reply(msgbuf.data(), msgbuf.size());
        socket_->send(reply);
    }

    void doRPCRequest(const rpc::Request& msg)
    {
        rpc::Reply rpc_rep = rpc_dispatcher_.requested(msg);
        this->sendMsg(rpc_rep);
    }

    template <typename Request, typename Reply>
    ProtobufRPCDispatcher& on(const typename ProtoRPCCallbackHolder<Request, Reply>::Callback& callback)
    {
        return rpc_dispatcher_.on<Request, Reply>(callback);
    }

protected:
    void on_recv(zmq::message_t& request)
    {
        // TODO:失败情况必须要返回
        MessageHeader* header = (MessageHeader*)request.data();

        std::cout << header->dumphex() << std::endl;

        if (header->msgsize() == request.size())
        {
            msg_dispatcher_instance_.dispatch(header);
        }
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t>  socket_;

    std::shared_ptr<std::thread> thread_;

    std::string broker_address_;

    ProtobufRPCDispatcher rpc_dispatcher_;
    ProtobufMsgDispatcherByName<> msg_dispatcher_instance_;

    bool isrunning_ = true;
};


#endif //TINYWORLD_ZMQ_SERVER_H
