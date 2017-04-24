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

#ifndef TINYWORLD_ZMQ_CLIENT_H
#define TINYWORLD_ZMQ_CLIENT_H

#include <zmq.hpp>
#include "tinylogger.h"
#include "message_dispatcher.h"

class ZMQClient {
public:
    typedef MessageNameDispatcher<> MsgDispatcher;

    ZMQClient(MsgDispatcher &dispatcher = MsgDispatcher::instance())
            : msg_dispatcher_(dispatcher) {
    }

    bool setID(const std::string &id) {
        if (id.length()) {
            socket_->setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
        }
        return true;
    }

    bool connect(const std::string &address) {
        LOG_TRACE("ZMQ", "Connecting to Server: %s", address.c_str());

        context_.reset(new zmq::context_t(1));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));
        socket_->connect(address);
        return true;
    }

    bool poll(long timeout = -1) {
        zmq::pollitem_t items[] = {{*socket_.get(), 0, ZMQ_POLLIN, 0}};

        int rc = zmq_poll(&items[0], 1, timeout);
        if (-1 != rc) {
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t empty;
                zmq::message_t request;

                socket_->recv(&empty);
                socket_->recv(&request);

                this->on_recv(request);
            }
        }

        return true;
    }

    //
    // Send Message
    //
    template<typename MsgT>
    void send(MsgT &msg) {
        MessageBuffer buffer;
        MsgDispatcher::template write2Buffer(buffer, msg);

        zmq::message_t empty;
        zmq::message_t message((const void *)buffer.data(), buffer.size());
        socket_->send(empty, ZMQ_SNDMORE);
        socket_->send(message);
    }

protected:
    //
    // Received Message
    //
    void on_recv(zmq::message_t &request) {
        try {
            msg_dispatcher_.dispatch(std::string((char*)request.data(), request.size()));
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


#endif //TINYWORLD_ZMQ_CLIENT_H
