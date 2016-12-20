#ifndef __COMMON_MYCACHE_H
#define __COMMON_MYCACHE_H

#include <string>
#include <map>
#include <list>
#include "zmq.hpp"
#include "pool.h"
#include "callback.h"
#include "mycache.pb.h"

enum MyCacheCmdEnum
{
	kMyCacheCmd_Null = 0,
	kMyCacheCmd_Get = 'g',  // get
	kMyCacheCmd_Set = 's',  // set
	kMyCacheCmd_Del = 'd',  // delete
};

class AsyncMyCacheClient;

namespace MyCache
{
	bool init(const std::string& address, unsigned int maxconns = 1);
	bool get(const std::string& key, std::string& value, int timeoutms = 1000);
	bool set(const std::string& key, const std::string& value, int timeoutms = 1000);
	bool del(const std::string& key, int timeoutms = 1000);


	class RPC;
	class Get;
	class Set;
	class Del;

	bool async_init(const std::string& address, const std::string& id = "");
	bool async_call(RPC* rpc, const ContextPtr& context, int timeoutms = -1);
	bool async_serials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool async_parallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
}

std::string formatKey(const char* format, ...);

namespace MyCache {

	bool packBinMsg(zmq::message_t& message, uint64_t id, uint8_t cmd, const std::string& msgbody);
	bool parseBinMsg(zmq::message_t& message, uint64_t& id, uint8_t& cmd, std::string& msgbody);

	inline bool sendBinMsg(zmq::socket_t* socket, uint64_t id, uint8_t cmd, const std::string& msg)
	{
		zmq::message_t message;
		if (packBinMsg(message, id, cmd, msg))
		{
			return socket->send(message);
		}
		
		return false;
	}

	template <typename T>
	inline bool sendMsg(zmq::socket_t* socket, uint64_t id, uint8_t cmd, const T* proto)
	{
		if (!socket) return false;

		std::string bin;
		if (proto && !proto->SerializeToString(&bin))
			return false;
		
		return sendBinMsg(socket, id, cmd, bin);
	}
}

/////////////////////////////////////////////////

class MyCacheClient
{
public:
	MyCacheClient(const std::string& id = "", zmq::context_t* context = NULL);

	~MyCacheClient();

	bool connect(const std::string& serveraddr);

	void close();

	bool request(std::string& reply, unsigned char cmdtype, const std::string& msg, int timeoutms = -1);

	bool get(const std::string& key, std::string& value, int timeoutms = 1000);
	bool set(const std::string& key, const std::string& value, int timeoutms = 1000);
	bool del(const std::string& key, int timeoutms = 1000);

	zmq::socket_t* socket() { return socket_; }

private:
	zmq::context_t* context_;
	zmq::socket_t*  socket_;

	std::string id_;
	std::string address_;
};

class MyCachePool : public ConnectionPoolWithLimit<MyCacheClient>
{
public:
	MyCachePool() {}

	virtual ~MyCachePool() {}

	static MyCachePool* instance() 
	{
		static MyCachePool mypool;
		return &mypool;
	}

	void setServerAddress(const std::string& serveraddr)
	{
		address_ = serveraddr;
	}

	virtual MyCacheClient* create()
	{
		MyCacheClient* client = new MyCacheClient();
		if (client && client->connect(address_))
		{
			return client;
		}

		return NULL;
	}

private:
	std::string address_;
};

typedef ScopedConnection<MyCacheClient, MyCachePool> ScopedMyCache;

/////////////////////////////////////////////////


class AsyncMyCacheClient
{
public:
	AsyncMyCacheClient(const std::string& id = "", zmq::context_t* context = NULL);

	~AsyncMyCacheClient();

	bool connect(const std::string& serveraddr);
	void close();
	void run();
	bool loop();

	bool call(MyCache::RPC* rpc, const ContextPtr& context, int timeoutms = -1);
	bool callBySerials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool callByParallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool callBySerials(SerialsCallBack* cb, const ContextPtr& context, MyCache::RPC* rpc1, ...);
	bool callByParallel(ParallelCallBack* cb, const ContextPtr& context, MyCache::RPC* rpc1, ...);

	bool get(MyCache::RPC* cb, const std::string& key, int timeoutms = 1000);
	bool set(MyCache::RPC* cb, const std::string& key, const std::string& value, int timeoutms = 1000);
	bool del(MyCache::RPC* cb, const std::string& key, int timeoutms = 1000);

	zmq::socket_t* socket() { return socket_; }
	AsyncRPCScheduler& scheduler() { return scheduler_; }

public:
	void onIdle();
	void onRecv(uint64_t id, uint8_t cmd, std::string& msgbody);

private:
	zmq::context_t* context_;
	zmq::socket_t*  socket_;

	std::string id_;
	std::string address_;

	AsyncRPCScheduler scheduler_;
};

namespace MyCache {

	class RPC : public AsyncRPC
	{
	public:
		RPC()
		{
			client = NULL;
		} 

		virtual ~RPC() {}

		template <typename T>
		bool sendToMycached(uint8_t cmd, const T* proto = NULL)
		{
			if (client)
				return MyCache::sendMsg<T>(client->socket(), id, cmd, proto);
			return false;
		}

		bool sendToMycached(uint8_t cmd, const std::string& msg)
		{
			if (client)
				return MyCache::sendBinMsg(client->socket(), id, cmd, msg);
			return false;
		}

		virtual void called(void* data)
		{
			if (data)
			{
				std::string& reply = *((std::string*)data);
				replied(reply);	
			}
		}

		virtual void replied(const std::string& reply) = 0;

	public:
		AsyncMyCacheClient* client;
	};

	class Get : public RPC
	{
	public:
		Get(const std::string& _key) : key(_key) {}

		virtual ~Get()
		{
			std::cout << "get: " << key << " -- over" << std::endl;
		}

		virtual void request()
		{
			std::cout << "get: " << key << " -- request" << std::endl;

			sendToMycached(kMyCacheCmd_Get, key);
		}

		virtual void replied(const std::string& reply)
		{
			std::cout << "get: " << key << " -- called" << std::endl;

			if (context)
				context->set(key, reply);
		}

	protected:
		std::string key;
	};

	class Del : public RPC
	{
	public:
		Del(const std::string& _key) : key(_key) {}

		virtual void request()
		{
			sendToMycached(kMyCacheCmd_Del, key);
		}

	protected:
		std::string key;
	};

	class Set : public RPC
	{
	public:
		Set(const std::string& key, const std::string& value)
		{
			msg.set_key(key);
			msg.set_value(value);
		}

		virtual void request()
		{
			std::string binmsg;
			if (msg.SerializeToString(&binmsg))
			{
				sendToMycached(kMyCacheCmd_Set, binmsg);
			}
		}

	protected:
		Cmd::Set msg;
	};

}

#if (!defined(__WINDOWS__))
#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))
#else
#define within(num) (int) ((float) (num) * rand () / (RAND_MAX + 1.0))
#endif


#endif // __COMMON_MYCACHE_H