#include "../common/mycache2.h"
#include "../common/mylogger.h"
#include <boost/lexical_cast.hpp>

static LoggerPtr logger(Logger::getLogger("mycache"));

using mycache::KVMap;

#include "mycache2_data.h"

void dump(const UserT& u)
{
	LOG4CXX_INFO(logger, "charid=" << u.charid << ",name=" << u.name << ",sex=" << u.sex << ", age=" << u.age);
}


void test_async(const std::string& arg, int num)
{
	mycache::async->connect(mycache::LV_LOCAL, "redis://127.0.0.1:6379");
	mycache::async->connect(mycache::LV_GLOBAL, "redis://127.0.0.1:6380");

	boost::thread_group thrds;

	for (int i = 0; i < num; ++i)
	{
		std::ostringstream oss;
		oss << "test:" << i;
		std::string key = oss.str();

		if ("set" == arg)
		{
			struct SetUser : public mycache::Set<UserT>
			{
				void result(bool result)
				{
					LOG4CXX_INFO(logger, "setuser: " << key_ << " " << result);
				}
			};

			UserT u;
			u.charid = i;
			u.name = key;
			u.sex = 1;
			u.age = i+10;
			mycache::async->set_to(new SetUser, key, u, mycache::LV_GLOBAL);
		}
		else if ("get" == arg)
		{
			struct GetUser : public mycache::Get<UserT>
			{
				void result(const UserT* user)
				{
					if (user)
					{
						dump(*user);
					}
					else
					{
						LOG4CXX_WARN(logger, "missing..");
					}
				}
			};

			mycache::async->get_from(new GetUser, key, mycache::LV_LOCAL);
			mycache::async->get_from(new GetUser, key, mycache::LV_GLOBAL);
		}
		else if ("del" == arg)
		{
			struct DelUser : public mycache::Del
			{
				void result(bool result)
				{
					LOG4CXX_INFO(logger, "deluser: " << key_ << " " << result);
				}
			};

			mycache::async->del_from(new DelUser, key, mycache::LV_GLOBAL);
		}
		else if ("get2" == arg)
		{
			struct GetUser : public mycache::Get<UserT>
			{
				void result(const UserT* user)
				{
					if (user)
					{
						dump(*user);
					}
					else
					{
						LOG4CXX_WARN(logger, "missing..");
					}
				}
			};

			mycache::async->get(new GetUser, key);
		}
		else if ("get3" == arg)
		{
			struct GetUser : public mycache::Get<UserT>
			{
				void result(const UserT* user)
				{
					if (user)
					{
						dump(*user);
					}
					else
					{
						LOG4CXX_WARN(logger, "missing..");
					}
				}
			};

			mycache::async->get_and_cache(new GetUser, key, mycache::LV_LOCAL);
		}
		else if ("del2" == arg)
		{
			mycache::async->del(key);
		}
	}

	EventLoop::instance()->loop();

	thrds.join_all();
}


int main(int argc, const char* argv[])
{
	BasicConfigurator::configure();

	if (argc > 1)
	{
		std::string arg1 = argv[1];
		std::string arg2 = (argc > 2 ? argv[2] : "");
		int num = (argc > 3 ? atol(argv[3]) : 1);

		if ("sync" == arg1)
		{
			mycache::sync->connect(mycache::LV_LOCAL, "redis://127.0.0.1:6379/0");
			mycache::sync->connect(mycache::LV_GLOBAL, "redis://127.0.0.1:6379/1");


			{
				LOG4CXX_INFO(logger, "set------------");

				UserT u;
				u.charid = 1;
				u.name = "david";
				u.sex = 1;
				u.age = 29;

				mycache::sync->set("test:1", u, mycache::LV_INPROC);
				mycache::sync->set("test:1", u, mycache::LV_LOCAL);

				u.charid = 2;
				mycache::sync->set("test:2", u, mycache::LV_GLOBAL);
				mycache::sync->dump_inproc();
			}

			{
				LOG4CXX_INFO(logger, "get------------");

				UserT u;
				mycache::sync->get("test:1", u);
				dump(u);

				mycache::sync->get_and_cache("test:2", u, mycache::LV_LOCAL);
				dump(u);

				mycache::sync->dump_inproc();
			}

			{
				LOG4CXX_INFO(logger, "del------------");

				mycache::sync->del("test:2");
				mycache::sync->dump_inproc();
			}

			mycache::sync->close();
		}
		else if ("async" == arg1)
		{
			test_async(arg2, num);
		}
	}
}

