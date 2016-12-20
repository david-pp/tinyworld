#ifndef __COMMON_REDIS_SYNC_H
#define __COMMON_REDIS_SYNC_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include <cstdarg>
#include "hiredis/hiredis.h"

struct RedisReply
{
public:
	typedef std::string String;
	typedef std::vector<String> Array;
	typedef std::map<std::string, std::string> Map;

	RedisReply();

	void reset();
	bool parseFrom(redisReply* reply);

	bool isNil() { return REDIS_REPLY_NIL == type; }
	long long getInteger() { return integer; }
	String& getStr() { return str; }
	Array& getArray() { return elements; }

	std::string debugString() const;
	std::string typeString() const;

public:
	int type;
	long long integer;
	String str;
	Array elements;
};

class RedisClient
{
public:
	RedisClient(const char* ip, int port);
	RedisClient(const std::string& url);
	
	~RedisClient();

	int shard() const { return shard_; }

	bool isConnected() { return context_ && context_->err == 0; }

	bool connect();
	bool connectWithTimeOut(int millsecs);
	void close();

	bool command(RedisReply& reply, const char* format, ...);
	bool command_v(RedisReply& reply, const char* format, va_list ap);
	bool command_argv(RedisReply& reply, const std::vector<std::string>& args);

	// pipeline
	bool appendCommand(const char* format, ...);
	bool getReply(RedisReply& reply);

public:
	bool select(int index);

	bool del(const std::string& key);

	// PUB/SUB
	int publish(const std::string& channel, const std::string& message);
	bool subscribe(const char* channel, ...);
	bool psubscribe(const char* channel, ...);
	bool unsubscribe(const char* channel, ...);
	bool punsubscribe(const char* channel, ...);

	// HASHES
	bool hgetall(const std::string& key, RedisReply::Map& kvs);
	bool hmset(const std::string& key, const RedisReply::Map& kvs);

private:
	void checkConnection();

	redisContext* context_;

	std::string url_;

	std::string ip_;
	int port_;
	int index_;

	int shard_;
};

typedef RedisClient RedisConnection;


#endif // __COMMON_REDIS_SYNC_H