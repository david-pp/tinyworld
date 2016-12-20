#ifndef __COMMON_MYREDIS_POOL_H
#define __COMMON_MYREDIS_POOL_H

#include "myredis.h"
#include "sharding.h"
#include "pool.h"
#include "url.h"

class RedisConnectionPool : public ConnectionPoolWithLimit<RedisConnection>
{
public:
	RedisConnectionPool()
	{
		shard_ = -1;
	}

	~RedisConnectionPool() {}

	static RedisConnectionPool* instance() 
	{
		static RedisConnectionPool redispool;
		return &redispool;
	}

public:
	// URL query paramaters
	//  shard   - shard id
	//  maxconn - maxconnctions
	void setServerAddress(const std::string& url);

	int shard() const { return shard_; }

	const std::string& url() const { return  url_; }

	virtual RedisConnection* create()
	{
		RedisConnection* conn = new RedisConnection(url_);
		if (conn)
		{
			conn->connect();
		}

		return conn;
	}

private:	
	// connection parameters
	std::string url_;
	// shard id
	int shard_;
};


class RedisShardingPool : public ShardingConnectionPool<RedisConnection, RedisConnectionPool>
{
public:
	typedef ShardingConnectionPool<RedisConnection, RedisConnectionPool> Base;

	RedisShardingPool(int shardnum = 1) 
	{
		Base::setShardNum(shardnum);
	}

	~RedisShardingPool() 
	{
		fini();
	}

	static RedisShardingPool* instance();

public:
	bool init();
	void fini();

	bool addShardings(const std::vector<std::string>& urls);
	bool addSharding(const std::string& url);
};


typedef ScopedConnection<RedisConnection, RedisConnectionPool> ScopedRedisConnection;
typedef ScopedConnectionByShard<RedisConnection, RedisShardingPool> RedisConnectionByShard;
typedef ScopedConnectionByHash<RedisConnection, RedisShardingPool> RedisConnectionByHash;
typedef ScopedConnectionByKey<RedisConnection, RedisShardingPool> RedisConnectionByKey;

#endif // __COMMON_MYREDIS_POOL_H
