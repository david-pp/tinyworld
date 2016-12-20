#include <iostream>
#include <cstdio>
#include <map>
#include <ev.h>

#include "../common/redis_async.h"

AsyncRedisClient cli("127.0.0.1", 6379);

struct Get : public RedisCmdCallBack
{
	std::string key;

	Get(const std::string& varname, const std::string& keyname) 
	{ 
		context_varname = varname; 
		key = keyname;
	}

	void request()
	{
		cli.command(this, "GET %s", key.c_str());
	}

	void called()
	{
		context->set(context_varname, reply.getStr());

		std::cout << reply.debugString() << std::endl;

		AsyncCallBack::called();
	}
};


void test_serials()
{
	Context* context = new Context;
	context->set("caller", 1);

	//AsyncCallBack::call(new Get("myname", "name"), context);

	struct Done : public AsyncRedisCallBack
	{
		void called()
		{
			std::cout << context->debugString() << std::endl;
			std::cout << context->get<std::string>("getname") << std::endl;
			std::cout << context->get<std::string>("getkey") << std::endl;
			std::cout << context->get<std::string>("getfoo") << std::endl;
			std::cout << context->get<int>("caller") << std::endl;
		}
	};

	AsyncCallBack::parallel(new Done,
	//AsyncCallBack::serials(new Done, 
		ACBS(
			new Get("getname", "name"),
			new Get("getkey", "key"),
			new Get("getfoo", "foo"),
			NULL
		),
		context);
}


#define DO(CB) \
struct CB : public AsyncRedisCallBack { \
	void called() \


#define WHEN(CB, CMD) \
}; \
cli.command(new CB, CMD); 


ev_timer mytimer;


void test_1()
{
	Context context;
	context["caller"] = std::string("on_timer");
	context["redis"] = &cli;

	struct GetCB : public AsyncRedisCallBack
	{
		GetCB(Context c) : context(c) {}
		Context context;
		void called()
		{
			context["name"] = reply.getStr();
			//std::cout << reply.debugString() << std::endl;

			struct Keys : public AsyncRedisCallBack
			{
				Keys(Context c) : context(c) {}
				Context context;
				void called()
				{
					context["keys"] = reply.getArray();
					//std::cout << reply.debugString() << std::endl;

					std::cout << "---------" << std::endl;
					std::cout << "name:" << context.get<std::string>("name") << std::endl;
					std::cout << "keys:" << context.get<std::vector<std::string> >("keys").size() << std::endl;

					context.get<AsyncRedisClient*>("redis")->close();
				}
			};

			cli.command(new Keys(context), "KEYS *");
		}


	};

	cli.command(new GetCB(context), "GET %s", "name");
}

void test_2()
{

}

void on_timer()
{
	std::cout << "timer ..." << std::endl;

	//test_1();
	//test_2();
	test_serials();


	#if 0
	cli.cmd<Get>("GET %s", "name");

	DO(GetName)
	{
		std::cout << "name:" << reply.getStr() << std::endl;

		DO(Keys)
		{
			std::cout << "Keys:" << reply.debugString() << std::endl;
		} WHEN(Keys, "KEYS *");

	} WHEN(GetName, "GET name");
	#endif
}

static void one_minute_cb (struct ev_loop *loop, ev_timer *w, int revents)
{
	on_timer();
	ev_timer_init (&mytimer, one_minute_cb, 1., 0.);
	ev_timer_start (cli.eventLoop(), &mytimer);
}

void reg_timer()
{
	ev_timer_init (&mytimer, one_minute_cb, 1., 0.);
	ev_timer_start (cli.eventLoop(), &mytimer);
}

void test_sub()
{
	DO(Subscribe)
	{
		std::cout << "sub:" << reply.debugString() << std::endl;
	} WHEN(Subscribe, "SUBSCRIBE mychannel");
}

int main(int argc, const char* argv[])
{
	//cli.connect();
	reg_timer();
	cli.eventRun();
}