#include "myredis_pool.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("redis"));

//////////////////////////////////////////////////////////////

void RedisConnectionPool::setServerAddress(const std::string& urltext)
{
	url_ = urltext;
	shard_ = -1;

	URL url;
	if (url.parse(urltext))
	{
		if (url.query["shard"].size() > 0)
		{
			shard_ = atol(url.query["shard"].c_str());
		}

		if (url.query["maxconn"].size() > 0)
		{
			unsigned int maxconn = atol(url.query["maxconn"].c_str());
			if (maxconn > 0)
				setMaxConn(maxconn);
		}
	}
}


//////////////////////////////////////////////////////////////////////////

RedisShardingPool* RedisShardingPool::instance()
{
	static RedisShardingPool shardingpool;
	return &shardingpool;
}

bool RedisShardingPool::init()
{
    for (int i = 0; i < shardNum(); i++)
	{
		RedisConnectionByShard shard(i, this);
	}

	return true;
}

void RedisShardingPool::fini()
{
	for (Base::Shards::iterator it = shards_.begin(); it != shards_.end(); ++ it)
	{
		delete it->second;
		it->second = NULL;
	}

	shards_.clear();
}

bool RedisShardingPool::addShardings(const std::vector<std::string>& urls)
{
	for (size_t i = 0; i < urls.size(); ++i)
	{
		if (!addSharding(urls[i]))
		{
			fini();
			return false;
		}
	}

	return true;
}

bool RedisShardingPool::addSharding(const std::string& url)
{
	RedisConnectionPool* pool = new RedisConnectionPool();
	if (!pool)
	{
		LOG4CXX_ERROR(logger, "create connection failed : new Pool : " << url);
		return false;
	}

	pool->setServerAddress(url);

	if (!this->addShard(pool))
	{
		LOG4CXX_ERROR(logger, "create connection failed : addShard : " << url);
		delete pool;
		pool = NULL;
		return false;
	}

	return true;
}
