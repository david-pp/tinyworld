
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

    bool isTimeout()
    {
        return timeout_ms_ < 0 ? false : elapsed_ms() > timeout_ms_;
    }

    void setTimeout(long ms)
    {
        timeout_ms_ = ms;
    }

    virtual void called(void* data) = 0;
    virtual void timeouted() {}


public:
    static uint64 total_id_;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

    /// 唯一标识
    uint64    id_;
    /// 创建时间点
    TimePoint createtime_;
    /// 超时时长
    long      timeout_ms_;
};

typedef std::shared_ptr<CallbackHolderBase> CallbackHolderPtr;


template <typename T>
struct GetCBHolder : public CallbackHolderBase
{
public:
    typedef T ObjectType;
    typedef typename T::KeyType KeyType;

    typedef std::function<void(const ObjectType& value)> Callback;
    typedef std::function<void(const KeyType& key)> ErrorCallback;

    GetCBHolder(const KeyType& key)
            : key_(key) {}

    virtual void called(void* data)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        ObjectType obj;
        KeyType key;
        if (cb_done_)
            cb_done_(obj);

        if (cb_timeout_)
            cb_timeout_(key);

        if (cb_nonexist_)
            cb_nonexist_(key);
    }

    GetCBHolder<T>& done(const Callback& cb)
    {
        cb_done_ = cb;
        return *this;
    }

    GetCBHolder<T>& timeout(const ErrorCallback& cb, long ms = 200)
    {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

    GetCBHolder<T>& nonexist(const ErrorCallback& cb)
    {
        cb_nonexist_ = cb;
        return *this;
    }

private:
    KeyType       key_;
    Callback      cb_done_;
    ErrorCallback cb_timeout_;
    ErrorCallback cb_nonexist_;
};


template <typename T>
struct SetCBHolder : public CallbackHolderBase
{
public:
    typedef T ObjectType;
    typedef typename T::KeyType KeyType;

    typedef std::function<void(const ObjectType& value)> Callback;
    typedef std::function<void(const ObjectType& value)> ErrorCallback;

    SetCBHolder(const KeyType& key, const ObjectType& obj)
            : key_(key), object_(obj) {}

    virtual void called(void* data)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        ObjectType obj;
        KeyType key;
        if (cb_done_)
            cb_done_(obj);

        if (cb_timeout_)
            cb_timeout_(obj);
    }

    SetCBHolder<T>& done(const Callback& cb)
    {
        cb_done_ = cb;
        return *this;
    }

    SetCBHolder<T>& timeout(const ErrorCallback& cb, long ms = 200)
    {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

private:
    KeyType       key_;
    ObjectType    object_;
    Callback      cb_done_;
    ErrorCallback cb_timeout_;
};

template <typename T>
struct DelCBHolder : public CallbackHolderBase
{
public:
    typedef T ObjectType;
    typedef typename T::KeyType KeyType;

    typedef std::function<void(const KeyType& key)> Callback;
    typedef std::function<void(const KeyType& key)> ErrorCallback;

    DelCBHolder(const KeyType& key)
            : key_(key){}

    virtual void called(void* data)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        ObjectType obj;
        KeyType key;
        if (cb_done_)
            cb_done_(key);

        if (cb_timeout_)
            cb_timeout_(key);
    }

    DelCBHolder<T>& done(const Callback& cb)
    {
        cb_done_ = cb;
        return *this;
    }

    DelCBHolder<T>& timeout(const ErrorCallback& cb, long ms = 200)
    {
        setTimeout(ms);
        cb_timeout_ = cb;
        return *this;
    }

private:
    KeyType       key_;
    ObjectType    object_;
    Callback      cb_done_;
    ErrorCallback cb_timeout_;
};


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


    template <typename T>
    GetCBHolder<T>& get(const typename T::KeyType& key)
    {
        GetCBHolder<T>* holder = new GetCBHolder<T>(key);
        CallbackHolderPtr holder_ptr(holder);
        callbacks_[holder->id_] = holder_ptr;
        return *holder;
    }

    template <typename T>
    SetCBHolder<T>& set(const T& value)
    {
        SetCBHolder<T>* holder = new SetCBHolder<T>(value.key(), value);
        CallbackHolderPtr holder_ptr(holder);
        callbacks_[holder->id_] = holder_ptr;
        return *holder;
    }

    template <typename T>
    DelCBHolder<T>& del(const typename T::KeyType& key)
    {
        DelCBHolder<T>* holder = new DelCBHolder<T>(key);
        CallbackHolderPtr holder_ptr(holder);
        callbacks_[holder->id_] = holder_ptr;
        return *holder;
    }

////        CallbackHolderPtr holder(new CallbackHolderT<T>(callback));
////        callbacks_.insert(std::make_pair(holder->id_, holder));
////
////        Cmd::Get send;
////        send.set_id_(holder->id_);
////        send.set_key(key);
////        sendCmd(send);


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


uint64 CallbackHolderBase::total_id_ = 0;


#include "tinyworld.h"
#include "mylogger.h"
#include "mydb.h"

#include "object2db.h"
#include "object2bin.h"

#include "player.h"
#include "player.db.h"
#include "player.pb.h"
#include "player.bin.h"

namespace tc {
    typedef Object2DB<Object2Bin<Player, PlayerBinDescriptor>, PlayerDBDescriptor> SuperPlayer;

    struct Player : public SuperPlayer {
        typedef uint32 KeyType;

        KeyType key() const { return id; }
    };
}


void test_api()
{
    std::shared_ptr<AsyncTinyCacheClient> tc(new AsyncTinyCacheClient);

    int local_value = 22;

    tc->get<tc::Player>(255)
            .done([&](const tc::Player& value){
                // 获取成功
                std::cout << "获取成功" << std::endl;

                tc->set<tc::Player>(value)
                        .done([](const tc::Player& value){
                            std::cout << "Set成功" << std::endl;
                        })
                        .timeout([tc](const tc::Player& value){
                            std::cout << "Set超时" << std::endl;

                            tc->del<tc::Player>(1111).called(NULL);

                        }, 600)
                        .called(NULL);
            })
            .nonexist([](const uint32& key){
                // 指定的玩家数据不存在
                std::cout << "指定玩家数据不存在" << std::endl;
            })
            .timeout([](const uint32& key){
                // 操作超时
                std::cout << "操作超时" << std::endl;
            })
            .called(NULL);
}

int main ()
{
    test_api();
    return 0;

    AsyncTinyCacheClient tc;
    tc.connect("tcp://localhost:5555");
    while(true)
    {
        tc.poll(1000);

    }
    return 0;
}