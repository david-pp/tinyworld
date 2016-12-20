#include "mydb.h"
#include "url.h"
#include <iostream>
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("mysql"));

//////////////////////////////////////////////////////////////////////////

bool MySqlConnection::connectByURL(const std::string& urltext)
{
	URL url;
	if (!url.parse(urltext))
	{
		LOG4CXX_ERROR(logger, "connect " << urltext << " failed : address is invalid");
		return false;
	}

	try 
	{
		this->connect(
			(url.path.size() > 0 ? url.path.c_str() : NULL),
			url.host.c_str(),
			url.username.c_str(),
			url.password.c_str(),
			url.port);

		if (url.query["shard"].size() > 0)
		{
			shard_ = atol(url.query["shard"].c_str());
		}

		LOG4CXX_INFO(logger, "connect " << urltext << " success");
	}
	catch (std::exception& er) 
	{
		LOG4CXX_ERROR(logger, "connect " << urltext << " failed : " << er.what());
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

MySqlConnectionPool::MySqlConnectionPool()
{
	wait_timeout_ = 28800; // MySQL default:8hours
	shard_ = -1;
}

void MySqlConnectionPool::setServerAddress(const std::string& urltext)
{
	url_ = urltext;
	URL url;
	if (url.parse(urltext))
	{
		if (url.query["shard"].size() > 0)
		{
			shard_ = atol(url.query["shard"].c_str());
		}

		if (url.query["idletime"].size() > 0)
		{
			wait_timeout_ = atol(url.query["idletime"].c_str());
			if (!wait_timeout_)
			{
				wait_timeout_ = 28800;
			}
		}

		if (url.query["maxconn"].size() > 0)
		{
			unsigned int maxconn = atol(url.query["maxconn"].c_str());
			if (maxconn > 0)
				setMaxConn(maxconn);
		}
	}
}
// Superclass overrides
mysqlpp::Connection* MySqlConnectionPool::create()
{
	// Create connection using the parameters we were passed upon
	// creation.  This could be something much more complex, but for
	// the purposes of the example, this suffices.
	MySqlConnection* conn = new MySqlConnection();
	if (conn)
	{
		conn->connectByURL(url_);
	}

	return conn;
}


//////////////////////////////////////////////////////////////////////////

MySqlShardingPool* MySqlShardingPool::instance()
{
	static MySqlShardingPool shardingpool;
	return &shardingpool;
}

bool MySqlShardingPool::init()
{
	for (int i = 0; i < shardNum(); i++)
	{
		MySqlConnectionByShard shard(i, this);
	}

	return true;
}

void MySqlShardingPool::fini()
{
	for (Base::Shards::iterator it = shards_.begin(); it != shards_.end(); ++ it)
	{
		delete it->second;
		it->second = NULL;
	}

	shards_.clear();
}

bool MySqlShardingPool::addShardings(const std::vector<std::string>& urls)
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

bool MySqlShardingPool::addSharding(const std::string& url)
{
	MySqlConnectionPool* pool = new MySqlConnectionPool();
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
