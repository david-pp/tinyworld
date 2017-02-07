
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

#include "message_dispatcher.h"
#include "command.pb.h"

//ProtobufMsgDispatcherByName dispatcher;
//
////  Prepare our context and sockets
//zmq::context_t* context = NULL;
//zmq::socket_t*  socket = NULL;
//
//
//void on_recv(zmq::message_t& request)
//{
//    MessageHeader* header = (MessageHeader*)request.data();
//
//    std::cout << header->dumphex() << std::endl;
//
//    if (header->msgsize() == request.size())
//    {
//        dispatcher.dispatch(header);
//    }
//}
//
//template <typename MSG>
//void sendMsg(MSG& proto)
//{
//    std::string msgbuf;
//    ProtobufMsgDispatcherByName::pack(msgbuf, &proto);
//    zmq::message_t reply(msgbuf.data(), msgbuf.size());
//
//    socket->send(reply);
//}
//
//void asyncclient(int i)
//{
//    //  Prepare our context and socket
//    try {
//        context = new zmq::context_t(1);
//        socket =  new zmq::socket_t(*context, ZMQ_DEALER);
//        std::ostringstream id;
//        id << "aclient" << i;
//        socket->setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());
//
//        std::cout << "Connecting to hello world server..." << std::endl;
//        socket->connect ("tcp://localhost:5555");
//
//        zmq::pollitem_t items[] = { { *socket, 0, ZMQ_POLLIN, 0 } };
//        while (true)
//        {
//            int rc = zmq_poll (&items[0], 1, 1000);
//            if (-1 != rc)
//            {
//                //  If we got a reply, process it
//                if (items[0].revents & ZMQ_POLLIN)
//                {
//                    zmq::message_t request;
//                    socket->recv (&request);
//
//                    on_recv(request);
//                }
//            }
//
//            Cmd::Get get;
//            get.set_type("CHARBASE");
//            get.set_key(2222);
//            sendMsg(get);
//        }
//    }
//    catch (std::exception& err)
//    {
//        std::cout << "!!!!" << err.what() << std::endl;
//    }
//}
//
//void onGetReply(Cmd::GetReply* msg)
//{
//    std::cout << msg->DebugString() << std::endl;
//}


class AsyncTinyCacheClient
{
public:
    AsyncTinyCacheClient()
    {
        dispatcher_.bind<Cmd::GetReply>(std::bind(&AsyncTinyCacheClient::onGetReply, this, std::placeholders::_1));

        context_.reset(new zmq::context_t(1));
        socket_.reset(new zmq::socket_t(*context_.get(), ZMQ_DEALER));

        std::ostringstream id;
        id << "aclient";
        socket_->setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());
    }

    bool connect(const std::string& address)
    {
        std::cout << "Connecting to hello world server..." << std::endl;
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

    struct CallbackHolderBase
    {
    public:
        CallbackHolderBase()
        {
            id_ = ++total_id_;
            createtime_ = std::chrono::high_resolution_clock::now();
        }

        long elapsed_ms()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - createtime_).count();
        }

        long elpased_ns()
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now() - createtime_).count();
        }

        virtual void called(void* data) = 0;
        virtual void timeouted() {}

    public:
        static uint64 total_id_;

        uint64 id_;
        std::chrono::time_point<std::chrono::high_resolution_clock> createtime_;
    };

    typedef std::shared_ptr<CallbackHolderBase> CallbackHolderPtr;



    template <typename T>
    struct CallbackHolderT : public CallbackHolderBase
    {
    public:
        typedef std::function<void(int retcode, const T& value)> Callback;

        CallbackHolderT(const Callback& cb)
                : callback_(cb) {}

        virtual void called(void* data)
        {
            T value;
            callback_(0, value);
        }

    private:
        Callback callback_;
    };


    template <typename T>
    bool get(uint32 key, const std::function<void(int retcode, const T& value)>& callback)
    {
        CallbackHolderPtr holder(new CallbackHolderT<T>(callback));
        callbacks_.insert(std::make_pair(holder->id_, holder));

        Cmd::Get send;
        send.set_id_(holder->id_);
        send.set_key(key);
        sendCmd(send);

        return true;
    }


    template <typename MSG>
    void sendCmd(MSG& proto)
    {
        std::string msgbuf;
        ProtobufMsgDispatcherByName::pack(msgbuf, &proto);
        zmq::message_t reply(msgbuf.data(), msgbuf.size());

        socket_->send(reply);
    }

protected:
    void on_recv(zmq::message_t& request)
    {
        MessageHeader* header = (MessageHeader*)request.data();

        std::cout << header->dumphex() << std::endl;

        if (header->msgsize() == request.size())
        {
            dispatcher_.dispatch(header);
        }
    }

    void onGetReply(Cmd::GetReply* msg)
    {
        auto it = callbacks_.find(msg->id_());
        if (it != callbacks_.end())
        {
            it->second->called(msg);
            std::cout << msg->DebugString() << std::endl;
        }
    }

private:
    std::shared_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t> socket_;

    typedef std::unordered_map<uint64, CallbackHolderPtr> CallbackHolderMap;
    CallbackHolderMap callbacks_;

    ProtobufMsgDispatcherByName dispatcher_;
};


uint64 AsyncTinyCacheClient::CallbackHolderBase::total_id_ = 0;

struct CharBase
{
};


int main () {
//    dispatcher.bind<Cmd::GetReply>(onGetReply);
//    asyncclient(1);

    AsyncTinyCacheClient tc;
    tc.connect("tcp://localhost:5555");
    while(true)
    {
        tc.poll(1000);

        tc.get<CharBase>(255, [](int retcode, const CharBase& value) {
            if (retcode == 0)
            {
            }
            std::cout << "callback...." << std::endl;
        });

    }
    return 0;
}