#include "mycache2.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("mycache"));


//////////////////////////////////////////////////////////////////////

bool SyncMyCache::connect(int level, const std::string& address)
{
	if (level <= 0) return false;

	boost::lock_guard<boost::mutex> guard(mutex_);

	RedisConnectionPool* pool = new RedisConnectionPool();
	if (pool)
	{
		pool->setServerAddress(address);
		pool->createAll();
		clients_[level] = pool;
		return true;
	}

	return false;
}

void SyncMyCache::close()
{
	boost::lock_guard<boost::mutex> guard(mutex_);

	for (Connections::iterator it = clients_.begin(); it != clients_.end(); ++ it)
	{
		it->second->removeAll();
		delete it->second;
	}
	clients_.clear();
}

RedisConnectionPool* SyncMyCache::client(int level)
{
	boost::lock_guard<boost::mutex> guard(mutex_);
	Connections::iterator it = clients_.find(level);
	if (it != clients_.end())
		return it->second;
	return NULL;
}

bool SyncMyCache::get_from_remote(mycache::KVMap& kvs, const std::string& key, int level)
{
	if (level <= 0) return false;

	ScopedRedisConnection redis(client(level));
	if (redis)
	{
		return redis->hgetall(key, kvs);
	}
	return false;
}

bool SyncMyCache::set_to_remote(const mycache::KVMap& kvs, const std::string& key, int level)
{
	if (level <= 0) return false;

	ScopedRedisConnection redis(client(level));
	if (redis)
	{
		return redis->hmset(key, kvs);
	}
	return false;
}

bool SyncMyCache::del_from_remote(const std::string& key, int level)
{
	if (level <= 0) return false;

	ScopedRedisConnection redis(client(level));
	if (redis)
	{
		return redis->del(key);
	}
	return false;
}

void SyncMyCache::dump_inproc()
{
	boost::lock_guard<boost::mutex> guard(cache_mutex_);
	for (L0Map::iterator it = cache_.begin(); it != cache_.end(); ++ it)
	{
		LOG4CXX_INFO(logger, "inproc : " << it->first);
	}
}


///////////////////////////////////////////////////////////////////


bool AsyncMyCache::connect(int level, const std::string& address, EventLoop* eventloop)
{
	if (level <= 0) return false;

	boost::lock_guard<boost::mutex> guard(mutex_);

	AsyncRedisClient* async = new AsyncRedisClient(address, eventloop);
	if (async && async->connect())
	{
		clients_[level] = async;
		return true;
	}

	return false;
}

void AsyncMyCache::close()
{
	boost::lock_guard<boost::mutex> guard(mutex_);

	for (Connections::iterator it = clients_.begin(); it != clients_.end(); ++ it)
	{
		it->second->close();
		delete it->second;
	}
	clients_.clear();
}

AsyncRedisClient* AsyncMyCache::client(int level)
{
	boost::lock_guard<boost::mutex> guard(mutex_);
	Connections::iterator it = clients_.find(level);
	if (it != clients_.end())
		return it->second;
	return NULL;
}

///////////////////////////////////////////////////////////////////

namespace mycache 
{
	SyncMyCache* sync = &SyncMyCache::instance();
	AsyncMyCache* async = &AsyncMyCache::instance();
}
