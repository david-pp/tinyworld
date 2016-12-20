#include "mydb.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("mycache-test"));

#define MAX_USER_NUM 100

bool initPool()
{
	MySqlShardingPool::instance()->setShardNum(3);
	MySqlShardingPool::instance()->addSharding("mysql://david:123456@127.0.0.1/shard0?shard=0");
   	MySqlShardingPool::instance()->addSharding("mysql://david:123456@127.0.0.1/shard1?shard=1");
  	MySqlShardingPool::instance()->addSharding("mysql://david:123456@127.0.0.1/shard2?shard=2");

  	if (MySqlShardingPool::instance()->isReady())
  	{
  		MySqlShardingPool::instance()->init();
  		return true;
  	}

  	return false;
}

void test_replace()
{
	for (int i = 0; i < MAX_USER_NUM; i++)
	{
		std::ostringstream oss;
		oss << "user:" << i;

		std::string key = oss.str();
		std::string value = "test_replace.......................!";

		try 
		{
			MySqlConnectionByKey mysql(key);
			if (mysql)
			{
				mysqlpp::Query query = mysql->query();
				query.reset();
				query << "replace into kvs_0"  
				      << "(`key`, value, flag, touchtime) values(" 
					  << mysqlpp::quote << key << ","
					  << mysqlpp::quote << value << ","
					  << 0 << ","
					  << time(0) << ")";
				query.execute();

				//LOG4CXX_INFO(logger, "replace: " << key << "," << mysql->shard() << ", success");
				LOG4CXX_INFO(logger, "replace: " << key << ", success");

			}

		}
		catch (std::exception& err)
		{
			LOG4CXX_INFO(logger, "replace: " << key << ", failed:" << err.what());
		}
	}
}

void test_select()
{
	for (int i = 0; i < MAX_USER_NUM; i++)
	{
		std::ostringstream oss;
		oss << "user:" << i;

		std::string key = oss.str();
		std::string value;

		MySqlConnectionByKey mysql(key);
		if (mysql)
		{
			try 
			{
				mysqlpp::Query query = mysql->query();
				query << "select * from kvs_0 where `key`=" << mysqlpp::quote << key;
				mysqlpp::StoreQueryResult res = query.store();
				if (res)
				{
					if (res.num_rows() == 1)
					{
						value.assign((char*)res[0]["value"].data(), res[0]["value"].size());
						LOG4CXX_INFO(logger, "select: " << key << "," << value);
					}
				}
			}
			catch (std::exception& err)
			{
				LOG4CXX_WARN(logger, "select: " << key << ", mysql error:" << err.what());
			}
		}
	}
}

int main(int argc, const char* argv[])
{
	BasicConfigurator::configure();
  	
  	if (!initPool())
  	{
  		LOG4CXX_ERROR(logger, "pool is not ready !");
  		return 1;
  	}

	if (argc > 1)
	{
		std::string arg1 = argv[1];
		int num = (argc > 2 ? atol(argv[2]) : 1);

		boost::thread_group thrds;

		for (int i = 0; i < num; i++)
		{
			if ("select" == arg1)
			{
				thrds.create_thread(test_select);
			}
			else if ("replace" == arg1)
			{
				thrds.create_thread(test_replace);
			}
		}

		thrds.join_all();
	}
}
