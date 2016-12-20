#ifndef __COMMON_REDIS_H
#define __COMMON_REDIS_H

#include "callback.h"
#include "eventloop.h"
#include "myredis_sync.h"
#include "myredis_async.h"

class EventLoop;

namespace myredis 
{
	// connect to the redis
	bool init(const std::string& address, unsigned int maxconns = 1);
	
	// excute redis command with reply, thread-safe
	bool command(RedisReply& reply, const char* format, ...);
	bool commandv(RedisReply& reply, const char* format, va_list ap);

	//
	// Keys Command
	//
	bool del(const std::string& key);
	bool exists(const std::string& key);
	bool expire(const std::string& key, unsigned int secs);
	bool expireat(const std::string& key, unsigned int timestamp);
	bool pexpire(const std::string& key, unsigned int millsecs);
	bool pexpireat(const std::string& key, unsigned int millsecs_timestamp);
	int  ttl(const std::string& key);
	int  pttl(const std::string& key);

	//
	// Strings Command
	//
	bool get(std::string& value, const std::string& key);
	bool set(const std::string& key, const std::string& value);
	int  append(const std::string& key, const std::string& value);
	int  strlen(const std::string& key);	

	bool setex(const std::string& key, const std::string& value, unsigned int expiresecs);
	bool psetex(const std::string& key, const std::string& value, unsigned int expiremillsecs);
	bool setnx(const std::string& key, const std::string& value);


	bool setbit(const std::string& key, int offset, int value);
	bool getbit(const std::string& key, int offset);
	int  bitcount(const std::string& key, int start, int end);

	int  setrange(const std::string& key, const std::string& value, int offset);
	bool getrange( std::string& value, const std::string& key, int start, int end);

	int incr(const std::string& key);
	int incrby(const std::string& key, int increment);
	float incrbyfloat(const std::string& key, float increment);
	int decr(const std::string& key);
	int decrby(const std::string& key, int decrement);

	//
	// Hashes Command
	//
	typedef std::map<std::string, std::string> HashFields;
	typedef std::vector<std::string> Fields;
	typedef std::vector<std::string> Values;

	bool hset(const std::string& key, const std::string& field, const std::string& value);
	bool hmset(const std::string& key, const char* format, ...);
	bool hsetnx(const std::string& key, const std::string& field, const std::string& value);

	bool hget(std::string& value, const std::string& key, const std::string& field);
	bool hmget(Values& values, const std::string& key, const Fields& fields);
	bool hgetall(HashFields& fields, const std::string& key);
	
	bool hdel(const std::string& key, const Fields& fields);
	bool hexists(const std::string& key, const std::string& field);
	int  hlen(const std::string& key);
	bool hkeys(Fields& fields, const std::string& key);
	bool hvals(Values& values, const std::string& key);

	int  hincrby(const std::string& key, const std::string& field, int increment);
	float hincrbyfloat(const std::string& key, const std::string& field, float increment);

	//
	// Lists Command
	//
	bool lpush(const std::string& key, const char* format, ...);
	bool lpushx(const std::string& key, const std::string& value);
	bool lpop(std::string& value, const std::string& key);

	bool rpush(const std::string& key, const char* format, ...);
	bool rpushx(const std::string& key, const std::string& value);
	bool rpop(std::string& value, const std::string& key);

	bool lset(const std::string& key, int index, const std::string& value);
	bool lrem(const std::string& key, int count, const std::string& value);
	bool ltrim(const std::string& key, int start, int stop);
	bool linsert_before(const std::string& key, const std::string& pivot, const std::string& value);
	bool linsert_after(const std::string& key, const std::string& pivot, const std::string& value);

	bool lrange(Values& values, const std::string& key, int start = 0, int stop = -1);
	bool lindex(std::string& value, const std::string& key, int index);
	int  llen(const std::string& key);

	//
	// Set Commands
	//
	bool sadd(const std::string& key, const char* format, ...);
	bool saddone(const std::string& key, const std::string& value);
	bool smembers(Values& values, const std::string& key);
	bool spop(std::string& value, const std::string& key);
	bool rem(const std::string& key, const char* format, ...);
	bool remone(const std::string& key, const std::string& value);

	int  scard(const std::string& key);
	bool sismember(const std::string& key, const std::string& member);
}

namespace myredis
{
	class RPC;

	bool async_init(const std::string& address, EventLoop* eventloop = EventLoop::instance());
	bool async_call(RPC* rpc, const ContextPtr& context=ContextPtr(), int timeoutms = -1);
	bool async_serials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool async_parallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);

	std::string async_debugstr();
}

#endif // __COMMON_REDIS_H