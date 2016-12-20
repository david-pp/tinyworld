#ifndef __COMMON_REDIS_ASYNC_H
#define __COMMON_REDIS_ASYNC_H

#include <set>
#include <map>
#include <hiredis/async.h>
#include <ev.h>
#include "myredis.h"
#include "callback.h"
#include "eventloop.h"

class AsyncRedisClient;

namespace myredis
{
	struct RPC : public AsyncRPC
	{
	public:
		RPC()
		{
			client = NULL;
		}

		virtual void request() {}
		virtual void called(void* data) = 0;
		virtual void timeouted() {}

	public:
		AsyncRedisClient* client;
	};

	struct Command : public RPC
	{
	public:
		virtual void called(void* data)
		{
			if (data)
			{
				RedisReply reply;
				reply.parseFrom((redisReply*)data);

				if (context && varname.size())
					context->set(varname, reply);

				this->replied(reply);
			}
		}

		virtual void replied(RedisReply& reply) {}

	protected:
		std::string varname;
	};

	struct Callback
	{
	public:
		Callback()
		{
			client = NULL;
		}

		virtual void replied(RedisReply& reply) = 0;

	public:
		AsyncRedisClient* client;
	};
};


class AsyncRedisClient
{
public:
	AsyncRedisClient(const std::string& ip, int port, EventLoop* loop = EventLoop::instance());
	AsyncRedisClient(const std::string& url, EventLoop* loop = EventLoop::instance());
	~AsyncRedisClient();

	bool isConnected() { return context_ && context_->err == 0; }
	bool connect();
	void close();
	void run();
	AsyncRPCScheduler& scheduler() { return scheduler_; }

	// RPC MODE
	bool call(myredis::RPC* rpc, const ContextPtr& context=ContextPtr(), int timeoutms = -1);
	bool callBySerials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool callByParallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);

	// COMMAND MODE
	bool cmd(myredis::Callback* cb, const char* format, ...);
	bool cmd_v(myredis::Callback* cb, const char* format, va_list ap);
	bool cmd_argv(myredis::Callback* cb, const std::vector<std::string>& args);

public:
	bool command(myredis::RPC* callback, const char* format, ...);
	bool command_v(myredis::RPC* callback, const char* format, va_list ap);
	bool command_argv(myredis::RPC* callback, const std::vector<std::string>& args);

	void onConnected(bool success);
	void onDisconnected(bool success);

private:
	void checkConnection();
	void runByIdle();

	redisAsyncContext* context_;

	EventLoop* evloop_;

	std::string ip_;
	int port_;
	int db_;

	AsyncRPCScheduler scheduler_;
};


namespace myredis {

	struct Get : public Command
	{
	public:
		Get(const std::string& var, const std::string& key) : key_(key) 
		{
			varname = var;
		}
		Get(const std::string& key) : key_(key)
		{
			varname = key;
		}

		virtual void request()
		{
			if (client) client->command(this, "GET %s", key_.c_str());
		}

	protected:
		std::string key_;
	};

	struct Set : public Command
	{
	public:
		Set(const std::string& key, const std::string& value) 
			: key_(key), value_(value) {}

		virtual void request()
		{
			if (client) client->command(this, "SET %s %b", key_.c_str(), value_.data(), value_.size());
		}

	protected:
		std::string key_;
		std::string value_;
	};

	struct Del : public Command
	{
	public:
		Del(const std::string& key = "") : key_(key) {}

		virtual void request()
		{
			if (client) client->command(this, "DEL %s", key_.c_str());
		}
	protected:
		std::string key_;
	};

	struct HGetAll : public Command
	{
	public:
		HGetAll(const std::string& key = "") : key_(key) {}

		void setKey(const std::string& key)
		{
			key_ = key;
		}

		const std::string& getKey() { return key_; }

		virtual void request()
		{
			if (client) client->command(this, "HGETALL %s", key_.c_str());
		}

		virtual void replied(RedisReply& reply)
		{
			RedisReply::Map kvs;
			RedisReply::Array& elements = reply.getArray();
			if (elements.size() >= 2)
			{
				for (size_t i = 0; i < elements.size() - 1; i += 2)
					kvs[elements[i]] = elements[i+1];
			}

			replied_as_kvs(kvs);
		}

		virtual void replied_as_kvs(RedisReply::Map& kvs) {}

	protected:
		std::string key_;
	};

	struct HMSet : public Command
	{
	public:
		HMSet() {}
		HMSet(const std::string& key, RedisReply::Map& kvs) : key_(key), kvs_(kvs) {}

		virtual void request()
		{
			if (client) 
			{
				std::vector<std::string> args;
				args.push_back("HMSET");
				args.push_back(key_);
				for (RedisReply::Map::const_iterator it = kvs_.begin(); it != kvs_.end(); ++ it)
				{
					args.push_back(it->first);
					args.push_back(it->second);
				}

				client->command_argv(this, args);
			}
		}

	protected:
		std::string key_;
		RedisReply::Map kvs_;
	};
}

#endif // __COMMON_REDIS_ASYNC_H
