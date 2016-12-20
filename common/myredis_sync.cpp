#include "myredis_sync.h"
#include <sstream>
#include <cstdlib>
#include "url.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("myredis"));

RedisReply::RedisReply()
{
	reset();
}

void RedisReply::reset()
{
	type = 0;
	integer = 0;
	str = "";
	elements.clear();
}

bool RedisReply::parseFrom(redisReply* reply)
{
	if (!reply) return false;

	this->reset();
	
	type = reply->type;

	if (REDIS_REPLY_INTEGER == type)
		integer = reply->integer;

	if (REDIS_REPLY_ERROR == type 
		|| REDIS_REPLY_STRING == type
		|| REDIS_REPLY_STATUS == type)
		str.assign(reply->str, reply->len);

	if (REDIS_REPLY_ARRAY == type)
	{
		for (size_t i = 0; i < reply->elements; i++)
		{
			std::string element;
			element.assign(reply->element[i]->str, reply->element[i]->len);
			elements.push_back(element);
		}
	}
    
	return true;
}

std::string RedisReply::typeString() const
{
	switch (type)
	{
		case REDIS_REPLY_STRING : return "STRING";
		case REDIS_REPLY_ARRAY  : return "ARRAY";
		case REDIS_REPLY_INTEGER: return "INTEGER";
		case REDIS_REPLY_NIL    : return "NILL";
		case REDIS_REPLY_STATUS : return "STATUS";
		case REDIS_REPLY_ERROR  : return "ERROR";
	}
	return "UNKOWN";
}

std::string RedisReply::debugString() const
{
	std::ostringstream oss;
	oss << "[" << typeString() << "]"
		<< " integer:" << integer
		<< " str:" << str
		<< " elements:";

	for (size_t i = 0; i < elements.size(); i++)
		oss << elements[i] << ",";

	return oss.str();
}


RedisClient::RedisClient(const char* ip, int port)
{
	context_ = NULL;
	ip_ = ip;
	port_ = port;
	shard_ = -1;
	index_ = 0;
}

RedisClient::RedisClient(const std::string& urltext)
{
	context_ = NULL;
	port_ = 0;
	shard_ = -1;
	index_ = 0;
	url_ = urltext;

	URL url;
	if (url.parse(urltext))
	{
		ip_ = url.host;
		port_ = url.port;

		if (url.query["shard"].size() > 0)
		{
			shard_ = atol(url.query["shard"].c_str());
		}

		if (url.path.size())
		{
			index_ = atol(url.path.c_str());
		}
	}
}

RedisClient::~RedisClient()
{
	close();
}

bool RedisClient::connect()
{
	context_ = redisConnect(ip_.c_str(), port_);
	if (context_ == NULL)
	{
		LOG4CXX_ERROR(logger, "Connection error: can't allocate redis context");
		return false;
	}

    if (context_->err) 
    {
    	LOG4CXX_ERROR(logger, "Connection error: " << context_->errstr);
    	close();
    	return false;
    }

    if (index_ > 0)
    	select(index_);

    LOG4CXX_INFO(logger, "connect success: " << url_);

    return true;
}

bool RedisClient::connectWithTimeOut(int millsecs)
{
	struct timeval timeout;
	timeout.tv_sec = millsecs/1000;
	timeout.tv_usec = (millsecs%1000)*1000;

	context_ = redisConnectWithTimeout(ip_.c_str(), port_, timeout);
	if (context_ == NULL)
	{
		printf("Connection error: can't allocate redis context\n");
		return false;
	}

    if (context_->err) 
    {
    	printf("Connection error: %s\n", context_->errstr);
    	close();
    	return false;
    }

    if (index_ > 0)
    	select(index_);

    return true;
}

void RedisClient::close()
{
	if (context_) 
	{
		redisFree(context_);
		context_ = NULL;
	} 
}

void RedisClient::checkConnection()
{
	if (!isConnected())
		connect();
}

bool RedisClient::command(RedisReply& reply, const char* format, ...)
{
	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(reply, format, ap);
	va_end(ap);
	return ret;
}

bool RedisClient::command_v(RedisReply& reply, const char* format, va_list ap)
{
	checkConnection();
	if (!isConnected())
		return false;

	redisReply* redisreply = (redisReply*)redisvCommand(context_, format, ap);

    reply.parseFrom(redisreply);
    freeReplyObject(redisreply);
    return true;
}

bool RedisClient::command_argv(RedisReply& reply, const std::vector<std::string>& args)
{
	checkConnection();
	if (!isConnected())
		return false;

	int argc = (int)args.size();
	char** argv = new char* [args.size()];
	size_t* argvlen = new size_t [args.size()];
	for (size_t i = 0; i < args.size(); i++)
	{
		argv[i] = (char*)args[i].data();
		argvlen[i] = args[i].size();
	}

	redisReply* redisreply = (redisReply*)redisCommandArgv(context_, argc, (const char**)argv, argvlen);

    reply.parseFrom(redisreply);
    freeReplyObject(redisreply);

    delete [] argv;
	delete [] argvlen;

	return true;
}

bool RedisClient::appendCommand(const char* format, ...)
{
	checkConnection();
	if (!isConnected())
		return false;

	va_list ap;
    int ret;
    va_start(ap,format);
    ret = redisvAppendCommand(context_, format, ap);
    va_end(ap);

    return ret == REDIS_OK;
}

bool RedisClient::getReply(RedisReply& reply)
{
	checkConnection();
	if (!isConnected())
		return false;

	redisReply* redisreply;
	int ret = redisGetReply(context_, (void **)&redisreply);
	reply.parseFrom(redisreply);
	freeReplyObject(redisreply);
	return ret == REDIS_OK;
}

bool RedisClient::select(int index)
{
	RedisReply reply;
	command(reply, "SELECT %d", index);
	return reply.getStr() == "OK";
}

bool RedisClient::del(const std::string& key)
{
	RedisReply reply;
	command(reply, "DEL %s", key.c_str());
	return reply.getInteger() == 1;
}

int RedisClient::publish(const std::string& channel, const std::string& message)
{
	RedisReply reply;
	if (command(reply, "PUBLISH %b %b", 
		channel.data(), (size_t)channel.size(),
		message.data(), (size_t)message.size()))
	{
		return reply.getInteger(); 
	}

	return 0;
}

static std::string makeCmdFormat(const std::string& cmd, const char* format)
{
	std::string cmdformat = cmd;
	cmdformat += " ";
	cmdformat += format;
	return cmdformat;
}

bool RedisClient::subscribe(const char* format, ...)
{
	RedisReply reply;

	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(reply, makeCmdFormat("SUBSCRIBE", format).c_str(), ap);
	va_end(ap);
	return ret;
}

bool RedisClient::psubscribe(const char* format, ...)
{
	RedisReply reply;

	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(reply, makeCmdFormat("PSUBSCRIBE", format).c_str(), ap);
	va_end(ap);
	return ret;
}

bool RedisClient::unsubscribe(const char* format, ...)
{
	RedisReply reply;

	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(reply, makeCmdFormat("UNSUBSCRIBE", format).c_str(), ap);
	va_end(ap);
	return ret;
}

bool RedisClient::punsubscribe(const char* format, ...)
{
	RedisReply reply;

	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(reply, makeCmdFormat("PUNSUBSCRIBE", format).c_str(), ap);
	va_end(ap);
	return ret;
}

bool RedisClient::hgetall(const std::string& key, RedisReply::Map& kvs)
{
	RedisReply reply;
	if (command(reply, "HGETALL %b", key.data(), key.size()))
	{
		RedisReply::Array& elements = reply.getArray();
		if (elements.size() < 2)
			return false;

		for (size_t i = 0; i < elements.size() - 1; i += 2)
			kvs[elements[i]] = elements[i+1];

		return true;
	}

	return false;
}

static void map2vector(const RedisReply::Map& kvs, RedisReply::Array& vec)
{
	for (RedisReply::Map::const_iterator it = kvs.begin(); it != kvs.end(); ++ it)
	{
		vec.push_back(it->first);
		vec.push_back(it->second);
	}
}

bool RedisClient::hmset(const std::string& key, const RedisReply::Map& kvs)
{
	std::vector<std::string> args;
	args.push_back("HMSET");
	args.push_back(key);
	map2vector(kvs, args);

	RedisReply reply;
	if (command_argv(reply, args))
	{
		return reply.getStr() == "OK";
	}
	return false;
}


