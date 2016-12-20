#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <boost/thread.hpp>
#include "../common/mydb.h"
#include "../common/mycache.h"
#include "../common/mylogger.h"

static LoggerPtr logger(Logger::getLogger("mycache-test"));

static int s_interrupted = 0;

static void s_signal_handler (int signal_value)
{
	LOG4CXX_INFO(logger, "catch the signal : " << signal_value);
    
    s_interrupted = 1;
}

static void s_catch_signals (void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}


MyCacheClient mycache;

void test_set(int num, int size = 10)
{
	for (int i = 0; i < num; ++i)
	{
		std::string key = formatKey("user:%d", i);

		//std::ostringstream value;
		//value << "user ................... " << i;

		std::string value(size, 'A');
		if (mycache.set(key, value))
		{
			LOG4CXX_INFO(logger, "set " << key << " [OK]");
		}
		else
		{
			LOG4CXX_INFO(logger, "set " << key << " [FAILED]");
		}
	}
}

void test_get(int num)
{
	for (int i = 0; i < num; ++i)
	{
		std::string key = formatKey("user:%d", i);
		std::string value;
		
		if (mycache.get(key, value))
		{
			LOG4CXX_INFO(logger, key << "=" << value.size());
		}
		else
		{
			LOG4CXX_INFO(logger, "get " << key << " [FAILED]");
		}
	}
}

void test_del(int num)
{
	for (int i = 0; i < num; ++i)
	{
		std::string key = formatKey("user:%d", i);
		std::string value;
		
		if (mycache.del(key))
		{
			LOG4CXX_INFO(logger, "del " << key << " [OK]");
		}
		else
		{
			LOG4CXX_INFO(logger, "del " << key << " [FAILED]");
		}
	}
}

void test_thread(int num)
{
	#if 0
	MyCacheClient* cache = MyCachePool::instance()->acquire();
	if (cache)
	{
		LOG4CXX_INFO(logger, "do some thing:" << pthread_self());
		usleep(100);
		MyCachePool::instance()->putback(cache);
	}
	#endif

	#if 1
	for (int i = 0; i < num; ++i)
	{
		std::string key = formatKey("user:%d", i);
		std::string value;
		
		if (MyCache::get(key, value))
		{
			LOG4CXX_INFO(logger, key << "=" << value << ":" << pthread_self());
		}
		else
		{
			LOG4CXX_INFO(logger, "get " << key << " [FAILED]");
		}
	}
	#endif
}

void test_timer()
{
	MyCache::init("tcp://localhost:5555");
	while (!s_interrupted)
	{
		std::string reply;
		MyCache::get("user:1", reply);
		std::cout << reply << std::endl;
		sleep(1);
	}
}

void async_test_1(AsyncMyCacheClient* client)
{
	struct GetUser : public MyCache::Get
	{
		GetUser(const std::string& key) : MyCache::Get(key) {}

		virtual ~GetUser()
		{
			LOG4CXX_INFO(logger, id << " key1: deleted");
		}

		virtual void replied(const std::string& reply)
		{
			struct GetUser2 : public MyCache::Get
			{
				GetUser2(const std::string& key) :MyCache::Get(key) {}

				virtual ~GetUser2()
				{
					LOG4CXX_INFO(logger, id << " key2: deleted");
				}

				virtual void replied(const std::string& reply)
				{
					MyCache::Get::replied(reply);

					if (context)
					{
						LOG4CXX_INFO(logger, "serials: " 
							<< context->get<int>("worker") << ","
							<< context->get<std::string>("user:1") << ","
							<< context->get<std::string>("user:2"));
					}
					else
					{
						LOG4CXX_INFO(logger, "serials: no context");
					}
				}
			};

			LOG4CXX_INFO(logger, id << ":" <<  key << "=" << reply);
			MyCache::Get::replied(reply);

			client->call(new GetUser2("user:2"), context, 2000);	
		}

		virtual void timeouted()
		{
			LOG4CXX_INFO(logger, id << ":" << key << "= ... timeout");
		}
	};


	ContextPtr c(new Context());
	c->set("worker", 1);
	client->call(new GetUser("user:1"), c, 2000);
	//client->call(new GetUser("user:1"), ContextPtr(), 2000);
}

void async_test_2(AsyncMyCacheClient* client)
{

	struct Done : public SerialsCallBack
	{
		virtual void called(void*)
		{
			LOG4CXX_INFO(logger, "serials: " 
							<< context->get<int>("top") << ","
							<< context->get<std::string>("user:1") << ","
							<< context->get<std::string>("user:2") << ","
							<< context->get<std::string>("user:3") << ","
							);
		}
	};

	ContextPtr c(new Context);
	c->set("top", 2);

	client->callBySerials(new Done, 
		AsyncRPC::S(
			new MyCache::Get("user:1"),
			new MyCache::Get("user:2"),
			new MyCache::Get("user:3"),
			NULL), 
		c, -1);
}

void async_test_3(AsyncMyCacheClient* client)
{

	struct Done : public ParallelCallBack
	{
		virtual void called(void*)
		{
			LOG4CXX_INFO(logger, "parallel: " 
							<< context->get<int>("top") << ","
							<< context->get<std::string>("user:1") << ","
							<< context->get<std::string>("user:2") << ","
							<< context->get<std::string>("user:3") << ","
							);
		}
	};

	
	ContextPtr c(new Context);
	c->set("top", 2);

#if 0
	client->callByParallel(new Done, 
		AsyncRPC::S(
			new MyCache::Get("user:1"),
			new MyCache::Get("user:2"),
			new MyCache::Get("user:3"),
			NULL), 
		c, -1);
#else
	client->callByParallel(new Done, c, 
		new MyCache::Get("user:1"), 
		new MyCache::Get("user:2"), 
		NULL);
#endif
}


void async_worker(AsyncMyCacheClient* client)
{
	while (!s_interrupted)
	{		
		//async_test_1(client);
		//async_test_2(client);
		async_test_3(client);
		
		sleep(1);
		//break;
	}
}

void test_sharedptr()
{
	ContextPtr c(new Context);
	c->set("key1", "david");
	c->set("key2", 1);
	std::cout << c->debugString() << std::endl;

	{
	ContextPtr c2 = c;
	std::cout << c.use_count() << std::endl;
	}
	std::cout << c.use_count() << std::endl;

	return;
}

void test_async(int num)
{
	AsyncMyCacheClient async("test");

	if (!async.connect("tcp://localhost:5555"))
		return;

	boost::thread_group thrds;
	for (int i = 0; i < num ; i++)
		thrds.create_thread(boost::bind(async_worker, &async));

	while (!s_interrupted)
	{
		try 
		{
			async.loop();

			//std::cout << async.scheduler().stat().debugString() << std::endl;
        }
		catch (std::exception& err)
		{
			std::cout << "!!!!" << err.what() << std::endl;
		}
	}

	thrds.join_all();
}

int main(int argc, const char* argv[])
{
	s_catch_signals();
	BasicConfigurator::configure();

	mycache.connect("tcp://localhost:5555");

	if (argc > 1)
	{
		int num = 1;
		if (argc > 2)
			num = atol(argv[2]);

		int size = 10;
		if (argc > 3)
			size = atol(argv[3]);

		std::string arg1 = argv[1];
		if ("set" == arg1)
		{
			test_set(num, size);
		}
		else if("get" == arg1)
		{
			test_get(num);
		}
		else if ("del" == arg1)
		{
			test_del(num);
		}
		else if ("thread" == arg1)
		{
			//MyCachePool::instance()->setServerAddress("tcp://localhost:5555");
			//MyCachePool::instance()->setMaxConn(5);
			//MyCachePool::instance()->createAll();
			MyCache::init("tcp://localhost:5555", 5);

			boost::thread_group thrds;

			for (int i = 0; i < num; i++)
				thrds.create_thread(boost::bind(test_thread, 100));

			thrds.join_all();
		}
		else if ("timer" == arg1)
		{
			test_timer();
		}
		else if ("async" == arg1)
		{
			test_async(num);
		}

		return 0;
	}


}