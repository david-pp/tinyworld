#include "../common/myredis.h"
#include "../common/myredis_pool.h"
#include "../common/myredis_async.h"
#include "../common/mylogger.h"
#include "../common/eventloop.h"
#include <boost/lexical_cast.hpp>

static LoggerPtr logger(Logger::getLogger("myredis"));

#define MAX_USER_NUM 100

bool initPool()
{
	RedisShardingPool::instance()->setShardNum(3);
	RedisShardingPool::instance()->addSharding("redis://127.0.0.1:6379/0?shard=0");
	RedisShardingPool::instance()->addSharding("redis://127.0.0.1:6379/1?shard=1");
	RedisShardingPool::instance()->addSharding("redis://127.0.0.1:6379/2?shard=2");

	if (RedisShardingPool::instance()->isReady())
	{
		RedisShardingPool::instance()->init();
		return true;
	}

  	return false;
}

void test_get()
{
	for (int i = 0; i < MAX_USER_NUM; i++)
	{
		std::ostringstream oss;
		oss << "user:" << i;

		std::string key = oss.str();

		RedisConnectionByKey redis(key);
		if (redis)
		{
			RedisReply::Map kvs;
			if (redis->hgetall(key, kvs))
			{
				LOG4CXX_INFO(logger, "get " << key << " " << kvs["value"].size());
			}
			else
			{
				LOG4CXX_INFO(logger, "get " << key << " non-exists");
			}
		}
	}
}

void test_set()
{
	for (int i = 0; i < MAX_USER_NUM; i++)
	{
		std::ostringstream oss;
		oss << "user:" << i;

		char data[] = "fuckyou!\0...";

		std::string key = oss.str();
		std::string value(data, sizeof(data));

		RedisConnectionByKey redis(key);
		if (redis)
		{
			RedisReply::Map kvs;
			kvs["value"] = value;
			kvs["flag"] = boost::lexical_cast<std::string>(0);
			kvs["touchtime"] = boost::lexical_cast<std::string>(time(0));

			if (redis->hmset(key, kvs))
			{
				LOG4CXX_INFO(logger, "set " << key << " " << value.size() << " [OK]");
			}
			else
			{
				LOG4CXX_INFO(logger, "set " << key << " " << value.size() << " [FAILED]");
			}
		}
	}
}

void test_sharding(const std::string& arg, int num)
{ 	
  	if (!initPool())
  	{
  		LOG4CXX_ERROR(logger, "pool is not ready !");
  		return ;
  	}

  	boost::thread_group thrds;

	for (int i = 0; i < num; i++)
	{
		if ("get" == arg)
		{
			thrds.create_thread(test_get);
		}
		else if ("set" == arg)
		{
			thrds.create_thread(test_set);
		}
	}

	thrds.join_all();
}

////////////////////////////////////////////////////////////

struct DoSth : public TimerEvent
{
	void called()
	{
		std::cout << myredis::async_debugstr() << std::endl;
	}
};

void async_get()
{
	for (int i = 0; i < MAX_USER_NUM; ++ i)
	{
		struct GetName : public myredis::Get
		{
			GetName(const std::string& key) : myredis::Get(key, key){}

			void replied(RedisReply& reply)
			{
				std::cout << key_ << "=" << reply.getStr() << std::endl;
			}
		};

		std::ostringstream oss;
		oss << "user:" << i;

		std::string key = oss.str();

		myredis::async_call(new GetName(key));
	}
}

void async_set()
{
	for (int i = 0; i < MAX_USER_NUM; ++ i)
	{
		std::ostringstream oss;
		oss << "user:" << i;

		std::string key = oss.str();
		std::string value = "fuckyou.......................!";

		myredis::async_call(new myredis::Set(key, value), ContextPtr());
	}
}

void async_serials()
{
	sleep(3);
	for (int i = 0; i < MAX_USER_NUM; ++ i)
	{
		struct Done : public SerialsCallBack
		{
			void called(void*)
			{
				LOG4CXX_INFO(logger, "serials: " 
							<< context->get<int>("top") << ","
							<< context->get<RedisReply>("user:1").getStr() << ","
							<< context->get<RedisReply>("user:2").getStr() << ","
							<< context->get<RedisReply>("user:3").getStr() << ","
							);
			}
		};

		ContextPtr c(new Context);
		c->set("top", 2);
		myredis::async_serials(new Done, 
			AsyncRPC::S(
				new myredis::Get("user:1"),
				new myredis::Get("user:2"),
				new myredis::Get("user:3"),
				NULL), 
			c, -1);
	}
}

void async_parallel()
{
	sleep(3);
	for (int i = 0; i < MAX_USER_NUM; ++ i)
	{
		struct Done : public ParallelCallBack
		{
			void called(void*)
			{
				LOG4CXX_INFO(logger, "serials: " 
							<< context->get<int>("top") << ","
							<< context->get<RedisReply>("user:1").getStr() << ","
							<< context->get<RedisReply>("user:2").getStr() << ","
							<< context->get<RedisReply>("user:3").getStr() << ","
							);
			}
		};

		ContextPtr c(new Context);
		c->set("top", 2);
		myredis::async_parallel(new Done, 
			AsyncRPC::S(
				new myredis::Get("user:1"),
				new myredis::Get("user:2"),
				new myredis::Get("user:3"),
				NULL), 
			c, -1);
	}
}

void test_async(const std::string& arg, int num)
{
	myredis::async_init("redis://127.0.0.1:6379");

	boost::thread_group thrds;

	EventLoop::instance()->regTimer(new DoSth, 1, 2);

	for (int i = 0; i < num; ++i)
	{
		if ("set" == arg)
		{
			thrds.create_thread(async_set);
			
		}
		else if ("get" == arg)
		{
			thrds.create_thread(async_get);
		}
		else if ("serials" == arg)
		{
			thrds.create_thread(async_serials);
		}
		else if ("parallel" == arg)
		{
			thrds.create_thread(async_parallel);
		}
	}

	EventLoop::instance()->loop();

	thrds.join_all();
}

////////////////////////////////////////////////////////////


void test_syncapi(const std::string& arg, int num)
{
	myredis::init("redis://127.0.0.1:6379");

	for (int i = 0; i < num; i++)
	{
		std::ostringstream oss;
		oss << "sync:" << i;

		std::string key = oss.str();

		if ("set" == arg)
		{
			std::string value(1024*64, 'A');
			if (myredis::set(key, value))
			{
				std::cout << "set - " << key << "=" << value.size() << std::endl;
			}
		}
		else if ("get" == arg)
		{
			std::string value;
			if (myredis::get(value, key))
			{
				std::cout << "get - " << key << "=" << value.size() << std::endl;
			}
		}
		else if ("del" == arg)
		{
			if (myredis::del(key))
			{
				std::cout << "del - " << key  << std::endl;
			}
		}
	}
}

////////////////////////////////////////////////////////////

int main(int argc, const char* argv[])
{
	BasicConfigurator::configure();

	if (argc > 1)
	{
		std::string arg1 = argv[1];
		std::string arg2 = (argc > 2 ? argv[2] : "");
		int num = (argc > 3 ? atol(argv[3]) : 1);

		if ("sharding" == arg1)
		{
			test_sharding(arg2, num);
		}
		else if ("async" == arg1)
		{
			test_async(arg2, num);
		}
		else if ("syncapi" == arg1)
		{
			test_syncapi(arg2, num);
		}
	}
}
