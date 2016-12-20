#include "myredis.h"
#include "myredis_pool.h"

namespace myredis {

	bool init(const std::string& address, unsigned int maxconns)
	{
		RedisConnectionPool::instance()->setServerAddress(address);
		RedisConnectionPool::instance()->setMaxConn(maxconns);
		RedisConnectionPool::instance()->createAll();
		return true;
	}

	bool command(RedisReply& reply, const char* format, ...)
	{
		ScopedRedisConnection redis;
		if (redis)
		{
			bool ret = false;
			va_list ap;
			va_start(ap, format);
			ret = redis->command_v(reply, format, ap);
			va_end(ap);

			return ret;
		}

		return false;
	}

	bool commandv(RedisReply& reply, const char* format, va_list ap)
	{
		ScopedRedisConnection redis;
		if (redis)
		{
			redis->command_v(reply, format, ap);
		}
		return false;
	}

	// keys ....
	bool del(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "DEL %s", key.c_str()))
			return reply.getInteger() == 1;
		return false;
	}

	bool exists(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "EXISTS %s", key.c_str()))
			return reply.getInteger() == 1;
		return false;
	}

	bool expire(const std::string& key, unsigned int secs)
	{
		RedisReply reply;
		if (command(reply, "EXPIRE %s %d", key.c_str(), secs))
			return reply.getInteger() == 1;
		return false;
	}

	bool expireat(const std::string& key, unsigned int timestamp)
	{
		RedisReply reply;
		if (command(reply, "EXPIREAT %s %d", key.c_str(), timestamp))
			return reply.getInteger() == 1;
		return false;
	}

	bool pexpire(const std::string& key, unsigned int millsecs)
	{
		RedisReply reply;
		if (command(reply, "PEXPIRE %s %d", key.c_str(), millsecs))
			return reply.getInteger() == 1;
		return false;	
	}

	bool pexpireat(const std::string& key, unsigned int millsecs_timestamp)
	{
		RedisReply reply;
		if (command(reply, "PEXPIREAT %s %d", key.c_str(), millsecs_timestamp))
			return reply.getInteger() == 1;
		return false;
	}

	int  ttl(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "TTL %s", key.c_str()))
			return reply.getInteger();
		return 0;
	}

	int  pttl(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "PTTL %s", key.c_str()))
			return reply.getInteger();
		return 0;
	}

	// string ........

	bool get(std::string& value, const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "GET %s", key.c_str()))
		{
			value.assign(reply.getStr().data(), reply.getStr().size());
			return true;
		}
		return false;
	}

	bool set(const std::string& key, const std::string& value)
	{
		RedisReply reply;
		if (command(reply, "SET %s %b", key.c_str(), value.data(), value.size()))
			return reply.getStr() == "OK";
		return false;
	}

	int append(const std::string& key, const std::string& value)
	{
		RedisReply reply;
		if (command(reply, "APPEND %s %b", key.c_str(), value.data(), value.size()))
			return reply.getInteger();
		return 0;
	}

	int  strlen(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "STRLEN %s", key.c_str()))
			return reply.getInteger();
		return 0;
	}

	bool setex(const std::string& key, const std::string& value, unsigned int expiresecs)
	{
		RedisReply reply;
		if (command(reply, "SETEX %s %u %b", key.c_str(), expiresecs, value.data(), value.size()))
			return reply.getStr() == "OK";
		return false;
	}

	bool psetex(const std::string& key, const std::string& value, unsigned int expiremillsecs)
	{
		RedisReply reply;
		if (command(reply, "PSETEX %s %u %b", key.c_str(), expiremillsecs, value.data(), value.size()))
			return reply.getStr() == "OK";
		return false;
	}

	bool setnx(const std::string& key, const std::string& value)
	{
		RedisReply reply;
		if (command(reply, "SETNX %s %b", key.c_str(), value.data(), value.size()))
			return reply.getStr() == "OK";
		return false;	
	}


	bool setbit(const std::string& key, int offset, int value)
	{
		RedisReply reply;
		if (command(reply, "SETBIT %s %d %d", key.c_str(), offset, (value > 0 ? 1:0)))
			return reply.getInteger() == 1;
		return false;
	}

	bool getbit(const std::string& key, int offset)
	{
		RedisReply reply;
		if (command(reply, "GETBIT %s %d", key.c_str(), offset))
			return reply.getInteger() > 0;
		return false;
	}

	int  bitcount(const std::string& key, int start, int end)
	{
		RedisReply reply;
		if (command(reply, "BITCOUNT %s %d %d", key.c_str(), start, end))
			return reply.getInteger() > 0;
		return 0;
	}

	int setrange(const std::string& key, const std::string& value, int offset)
	{
		RedisReply reply;
		if (command(reply, "SETRANGE %s %d %b", key.c_str(), offset, value.data(), value.size()))
			return reply.getInteger();
		return 0;
	}

	bool getrange(std::string& value, const std::string& key, int start, int end)
	{
		RedisReply reply;
		if (command(reply, "GETRANGE %s %d %d", key.c_str(), start, end))
		{
			value.assign(reply.getStr().data(), reply.getStr().size());
			return true;
		}
		return false;
	}

	int incr(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "INCR %s", key.c_str()))
			return reply.getInteger();
		return 0;
	}

	int incrby(const std::string& key, int increment)
	{
		RedisReply reply;
		if (command(reply, "INCRBY %s %d", key.c_str(), increment))
			return reply.getInteger();
		return 0;
	}

	float incrbyfloat(const std::string& key, float increment)
	{
		RedisReply reply;
		if (command(reply, "INCRBYFLOAT %s %f", key.c_str(), increment))
			return atof(reply.getStr().c_str());
		return 0.0;
	}

	int decr(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "DECR %s", key.c_str()))
			return reply.getInteger();
		return 0;
	}

	int decrby(const std::string& key, int decrement)
	{
		RedisReply reply;
		if (command(reply, "DECRBY %s %d", key.c_str(), decrement))
			return reply.getInteger();
		return 0;
	}

	// hash
	bool hset(const std::string& key, const std::string& field, const std::string& value)
	{
		RedisReply reply;
		if (command(reply, "HSET %s %s %b", key.c_str(), field.c_str(), value.data(), value.size()))
			return reply.getInteger() == 0;
		return false;
	}

	static std::string makeCmdFormat(const std::string& cmd, const char* format)
	{
		std::string cmdformat = cmd;
		cmdformat += " ";
		cmdformat += format;
		return cmdformat;
	}

	static std::string makeValuesFormat(std::vector<std::string> values)
	{
		std::string ret;
		for (size_t i = 0; i < values.size(); i++)
		{
			ret += " ";
			ret += values[i];
		}
		return ret;
	}

	bool hmset(const std::string& key, const char* format, ...)
	{
		RedisReply reply;
		bool ret = false;
		va_list ap;
		va_start(ap, format);
		ret = commandv(reply, makeCmdFormat("hmset", format).c_str(), ap);
		va_end(ap);
		return ret && reply.getStr() == "OK";
	}

	bool hsetnx(const std::string& key, const std::string& field, const std::string& value)
	{
		RedisReply reply;
		if (command(reply, "HSETNX %s %s %b", key.c_str(), field.c_str(), value.data(), value.size()))
			return reply.getInteger() == 0;
		return false;
	}

	bool hget(std::string& value, const std::string& key, const std::string& field)
	{
		RedisReply reply;
		if (command(reply, "HGET %s %s", key.c_str(), field.c_str()))
		{
			value.assign(reply.getStr().data(), reply.getStr().size());
			return true;
		}
		return false;
	}

	bool hmget(Values& values, const std::string& key, const Fields& fields)
	{
		RedisReply reply;
		if (command(reply, "HMGET %s %s", key.c_str(), makeValuesFormat(fields).c_str()))
		{
			values = reply.getArray();
			return true;
		}
		return false;
	}

	bool hgetall(HashFields& fields, const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "HGETALL %s", key.c_str()))
		{
			if (reply.getArray().size() > 0)
			{
				for (size_t i = 0; i < reply.getArray().size() - 1 ; i += 2)
				{
					fields.insert(std::make_pair(reply.getArray()[i], reply.getArray()[i+1]));
				}
			}
			return true;
		}
		return false;
	}
	
	bool hdel(const std::string& key, const Fields& fields)
	{
		RedisReply reply;
		if (command(reply, "HDEL %s %s", key.c_str(), makeValuesFormat(fields).c_str()))
			return reply.getInteger() == 1;
		return false;
	}

	bool hexists(const std::string& key, const std::string& field)
	{
		RedisReply reply;
		if (command(reply, "HEXISTS %s %s", key.c_str(), field.c_str()))
			return reply.getInteger() == 1;
		return false;
	}

	int  hlen(const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "HLEN %s", key.c_str()))
			return reply.getInteger();
		return false;
	}

	bool hkeys(Fields& fields, const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "HKEYS %s", key.c_str()))
		{
			fields = reply.getArray();
			return true;
		}
		return false;
	}

	bool hvals(Values& values, const std::string& key)
	{
		RedisReply reply;
		if (command(reply, "HVALS %s", key.c_str()))
		{
			values = reply.getArray();
			return true;
		}
		return false;
	}

	int hincrby(const std::string& key, const std::string& field, int increment)
	{
		RedisReply reply;
		if (command(reply, "HINCRBY %s %s %d", key.c_str(), field.c_str(), increment))
			return reply.getInteger();
		return 0;
	}

	float hincrbyfloat(const std::string& key, const std::string& field, float increment)
	{
		RedisReply reply;
		if (command(reply, "HINCRBY %s %s %f", key.c_str(), field.c_str(), increment))
			return atof(reply.getStr().c_str());
		return 0;
	}
}

namespace myredis {

	static AsyncRedisClient* async = NULL;

	bool async_init(const std::string& address, EventLoop* eventloop)
	{
		async = new AsyncRedisClient(address, eventloop);
		if (async)
		{
			return async->connect();
		}

		return false;
	}

	bool async_call(RPC* rpc, const ContextPtr& context, int timeoutms)
	{
		if (async)
			return async->call(rpc, context, timeoutms);
		return false;
	}

	bool async_serials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
	{
		if (async)
			return async->callBySerials(cb, rpcs, context, timeoutms);
		return false;
	}

	bool async_parallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
	{
		if (async)
			return async->callByParallel(cb, rpcs, context, timeoutms);
		return false;
	}

	std::string async_debugstr()
	{
		if (async)
			return async->scheduler().stat().debugString();
		return "";
	}
}
