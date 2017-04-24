// Copyright (c) 2017 david++
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TINYWORLD_ZMQ_SERVER_H
#define TINYWORLD_ZMQ_SERVER_H

#include <thread>
#include <iostream>
#include <string>
#include <iomanip>

#include <zmq.hpp>

#include "tinylogger.h"
#include "message_dispatcher.h"


inline void dumpmsg(zmq::message_t &message) {
    //  Dump the message as text or binary
    int size = message.size();
    std::string data(static_cast<char *>(message.data()), size);

    bool is_text = true;

    int char_nbr;
    unsigned char byte;
    for (char_nbr = 0; char_nbr < size; char_nbr++) {
        byte = data[char_nbr];
        if (byte < 32 || byte > 127)
            is_text = false;
    }
    std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";
    for (char_nbr = 0; char_nbr < size; char_nbr++) {
        if (is_text)
            std::cout << (char) data[char_nbr];
        else
            std::cout << std::setfill('0') << std::setw(2)
                      << std::hex << (unsigned int) data[char_nbr];
    }
    std::cout << std::endl;
}


//
// ZMQAsyncServer - Async Server
// ZMQBroker      - Broker
// ZMQWorker      - Sync/Worker Server
//

class ZMQAsyncServer {
public:
    typedef MessageNameDispatcher<const std::string &> MsgDispatcher;

    ZMQAsyncServer(MsgDispatcher &dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher) {
    }

    bool bind(const std::string &address) {
        LOG_TRACE("ZMQ", "Server listening : %s", address.c_str());

        context_.reset(new zmq::context_t(10));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_ROUTER));
        socket_->bind(address);
        return true;
    }

    bool poll(long timeout = -1) {
        zmq::pollitem_t items[] = {{*socket_.get(), 0, ZMQ_POLLIN, 0}};

        int rc = zmq_poll(&items[0], 1, timeout);
        if (-1 != rc) {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t idmsg;
                socket_->recv(&idmsg);
                std::string id((char *) idmsg.data(), idmsg.size());

                zmq::message_t empty;
                socket_->recv(&empty);

                zmq::message_t request;
                socket_->recv(&request);

                on_recv(id, request);
            }
        }

        return true;
    }

    //
    // Send Message to the specified client
    //
    template<typename MsgT>
    void send(const std::string &client, const MsgT &msg) {
        MessageBuffer buffer;
        MsgDispatcher::template write2Buffer(buffer, msg);

        zmq::message_t idmsg(client.data(), client.size());
        zmq::message_t empty;
        zmq::message_t reply(buffer.data(), buffer.size());

        socket_->send(idmsg, ZMQ_SNDMORE);
        socket_->send(empty, ZMQ_SNDMORE);
        socket_->send(reply);
    }

protected:
    void on_recv(std::string &client, zmq::message_t &request) {
        try {
            auto replybin = msg_dispatcher_.dispatch(std::string((char *) request.data(), request.size()), client);
            if (replybin) {
                zmq::message_t idmsg(client.data(), client.size());
                zmq::message_t empty;
                zmq::message_t reply(replybin->data(), replybin->size());
                socket_->send(idmsg, ZMQ_SNDMORE);
                socket_->send(empty, ZMQ_SNDMORE);
                socket_->send(reply);
            }
        }
        catch (std::exception &err) {
            LOG_ERROR("ZMQ", "recv: %s", err.what());
        }
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    MsgDispatcher &msg_dispatcher_;
};


class ZMQBroker {
public:
    ZMQBroker(std::shared_ptr<zmq::context_t> context = NULL) {
        if (context == NULL)
            context_.reset(new zmq::context_t(1));
        else
            context_ = context;
    }

    bool bind(const std::string &frontend, const std::string &backend) {
        LOG_TRACE("ZMQ", "Proxy Frontend listening : %s", frontend.c_str());
        LOG_TRACE("ZMQ", "Proxy Backend  listening : %s", backend.c_str());

        frontend_address_ = frontend;
        backend_address_ = backend;

        frontend_socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_ROUTER));
        backend_socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));

        frontend_socket_->bind(frontend_address_);
        backend_socket_->bind(backend_address_);

        return true;
    }

    void poll(long timeout = -1) {
        //  Initialize poll set
        zmq::pollitem_t items[] = {
                {*frontend_socket_.get(), 0, ZMQ_POLLIN, 0},
                {*backend_socket_.get(),  0, ZMQ_POLLIN, 0}
        };

        //  Switch messages between sockets
        if (true) {
            zmq::message_t message;
            int more;               //  Multipart detection

            zmq::poll(&items[0], 2, timeout);

            if (items[0].revents & ZMQ_POLLIN) {
                while (1) {
                    //  Process all parts of the message
                    frontend_socket_->recv(&message);
//                    dumpmsg(message);
                    LOGGER_DEBUG("ZMQBroker", "FRONTEND->BACKEND")
                    size_t more_size = sizeof(more);
                    frontend_socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
                    backend_socket_->send(message, more ? ZMQ_SNDMORE : 0);

                    if (!more)
                        break;      //  Last message part
                }
            }
            if (items[1].revents & ZMQ_POLLIN) {
                while (1) {
                    //  Process all parts of the message
                    backend_socket_->recv(&message);
//                    dumpmsg(message);
                    LOGGER_DEBUG("ZMQBroker", "BACKEND->FRONTEND")
                    size_t more_size = sizeof(more);
                    backend_socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
                    frontend_socket_->send(message, more ? ZMQ_SNDMORE : 0);
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
};

class ZMQWorker {
public:
    typedef MessageNameDispatcher<> MsgDispatcher;

    ZMQWorker(MsgDispatcher &dispatcher = MsgDispatcher::instance(),
              std::shared_ptr<zmq::context_t> context = NULL)
            : msg_dispatcher_(dispatcher) {
        if (context == NULL)
            context_.reset(new zmq::context_t(1));
        else
            context_ = context;
    }

    bool connect(const std::string &address) {
        if (!context_) return false;

        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_REP));
        socket_->connect(address);
        LOG_TRACE("ZMQ", "Connecting to Broker: %s", address.c_str());
        return true;
    }

    bool bind(const std::string &address) {
        if (!context_) return false;

        LOG_TRACE("ZMQ", "Server listening : %s", address.c_str());

        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_REP));
        socket_->bind(address);
        return true;
    }

    bool poll(long timeout = -1) {
        if (!context_ || !socket_) return false;

        zmq::pollitem_t items[] = {{*socket_.get(), 0, ZMQ_POLLIN, 0}};

        int rc = zmq_poll(&items[0], 1, timeout);
        if (-1 != rc) {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t request;
                socket_->recv(&request);
//                dumpmsg(request);
                this->on_recv(request);
            }
        }

        return true;
    }

    template<typename MsgT>
    void send(const MsgT &msg) {
        MessageBuffer msgbuf;
        MsgDispatcher::template write2Buffer(msgbuf, msg);
        zmq::message_t reply(msgbuf.data(), msgbuf.size());
        socket_->send(reply);
    }

protected:
    // must return a message
    void on_recv(zmq::message_t &request) {
        zmq::message_t err;

        try {
            auto replybin = msg_dispatcher_.dispatch(std::string((char*)request.data(), request.size()));
            if (replybin) {
                zmq::message_t reply(replybin->data(), replybin->size());
                socket_->send(reply);
                return;
            } else {
                LOG_ERROR("ZMQ", "msg handler has no reply");
            }
        }
        catch (std::exception &err) {
            LOG_ERROR("ZMQ", "%s", err.what());
        }

        socket_->send(err);
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    MsgDispatcher &msg_dispatcher_;
};

#endif //TINYWORLD_ZMQ_SERVER_H
