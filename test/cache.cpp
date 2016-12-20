#include <iostream>
#include <sstream>
#include "../common/mydb.h"
#include "../common/myredis.h"


struct MySimpleCache
{
public:
	MySimpleCache(RedisClient* r, mysqlpp::Connection* m) 
		: redis(r), mysql(m) {}

	bool get(const std::string& key, std::string& value)
	{
		RedisReply::Map kvs;
		if (redis->hgetall(key, kvs))
		{
			value = kvs["value"];
			std::cout << "hit ..." << std::endl;
			return true;
		}
		else
		{
			mysqlpp::Query query = mysql->query();
			query << "select * from kvs where `key`=" << mysqlpp::quote << key;
			mysqlpp::StoreQueryResult res = query.store();
			if (res)
			{
				if (res.num_rows() == 1)
				{
					value.assign(res[0]["value"].data(), res[0]["value"].size());
					redis->hmset("%b value %b flag %u touchtime %u", 
						key.data(), key.size(),
						value.data(), value.size(), 0, (unsigned int)time(0));

					std::cout << "miss ..." << std::endl;
					return true;
				}
			}
		}

		return false;
	}

	bool set(const std::string& key, const std::string& value)
	{
		redis->hmset("%s value %b flag %u touchtime %u", key.c_str(),
						value.data(), value.size(), 0, (unsigned int)time(0));

		mysqlpp::Query query = mysql->query();
		query << "replace into kvs(`key`, value, flag, touchtime) values(" 
			  << mysqlpp::quote << key << ","
			  << mysqlpp::quote << value << ","
			  << 0 << ","
			  << time(0) << ")";
		return true;
	}

	bool loadByKey(const std::string& key)
	{

	}

	bool loadByHotKeys()
	{
		return true;
	}

public:
	RedisClient* redis;
	mysqlpp::Connection* mysql;
};

using namespace std;

std::string dumphex(const char* data, int len)
{
	std::ostringstream oss;
	for (int i = 0; i < len; ++ i)
		oss << hex << (int)data[i] << ",";

	return oss.str();
}


void test_mycache()
{
	MySqlConnection con;
	con.connectByURL("mysql://david:123456@127.0.0.1/test_cache");

	RedisClient redis("redis://127.0.0.1:6379");

	MySimpleCache cache(&redis, &con);

	std::string value;
	
	while (true)
	{
		if (cache.get("name/3", value))
		{
			std::cout << dumphex(value.data(), value.size()) << std::endl;
		}

		sleep(1);
	}
}

void test_mysql()
{
	MySqlConnectionPool pool("mysql://david:123456@127.0.0.1/test_cache");

	while (1)
	{
		try {
		MySqlConnection* conn = pool.acquire();
		if (conn)
		{
			mysqlpp::Query query = conn->query("select * from kvs");
			mysqlpp::StoreQueryResult res = query.store();
			if (res)
			{
				for (size_t i = 0; i < res.num_rows(); ++i)
				{
					cout << res[i]["key"] << ","
						 << atol(res[i]["flag"].c_str())
						 << dumphex(res[i]["value"].data(), res[i]["value"].size())
						 << endl;
				}
			}

			pool.release(conn);
		}
		} catch(std::exception& err)
		{
			std::cout << err.what() << std::endl;
		}

		sleep(5);
	}
}


int install()
{
	// Connect to database server
	mysqlpp::Connection con;
	try {
		con.connect(0, "127.0.0.1", "david", "123456");
	}
	catch (exception& er) {
		cerr << "Connection failed: " << er.what() << endl;
		return 1;
	}
	
	if (true)
	{
		mysqlpp::NoExceptions ne(con);
		mysqlpp::Query query = con.query();
		if (con.select_db("test_cache")) {
		}
		else {
			// Database doesn't exist yet, so create and select it.
			if (con.create_db("test_cache") &&
					con.select_db("test_cache")) {
			}
			else {
				cerr << "Error creating DB: " << con.error() << endl;
				return 1;
			}
		}
	}

	try {
		// Send the query to create the stock table and execute it.
		std::cout << "Creating table..." << std::endl;
		mysqlpp::Query query = con.query();


		query.reset();
		query << "DROP TABLE kvs";
		query.execute();

		query.reset();
		query << 
				"CREATE TABLE kvs (" <<
				"  `key` VARCHAR(255) NOT NULL, " <<
				"  value BLOB NOT NULL," <<
				"  flag INT(10) UNSIGNED NOT NULL, " <<
				"  touchtime INT(10) UNSIGNED NOT NULL, "<<
				"PRIMARY KEY (`key`))" <<
				"ENGINE = InnoDB " <<
				"CHARACTER SET utf8 COLLATE utf8_general_ci";
		query.execute();

		for (int i = 0; i < 10; i++)
		{
			std::ostringstream key;
			key << "name/" << i;

			unsigned char bin[10] = {0x01,0x02,0x03,0x00,0x04,0x06,0x07};

			std::string value;
			value.assign((char*)&bin[0], 10);

			query.reset();
			query << "REPLACE INTO kvs VALUES(" 
				  << mysqlpp::quote << key.str() << ","
				  << mysqlpp::quote << value << ","
				  << 200 << ","
				  << time(0)
				  << ")";
			query.execute();
		}

		query.reset();
		query << "SELECT * FROM kvs";
		mysqlpp::StoreQueryResult res = query.store();
		if (res)
		{
			for (size_t i = 0; i < res.num_rows(); ++i)
			{
				cout << res[i]["key"] << ","
					 << atol(res[i]["flag"].c_str())
					 << dumphex(res[i]["value"].data(), res[i]["value"].size())
					 << endl;
			}
		}
	}
	catch (const mysqlpp::BadQuery& er) {
		// Handle any query errors
		cerr << endl << "Query error: " << er.what() << endl;
		return 1;
	}
	catch (const mysqlpp::BadConversion& er) {
		// Handle bad conversions
		cerr << endl << "Conversion error: " << er.what() << endl <<
				"\tretrieved data size: " << er.retrieved <<
				", actual size: " << er.actual_size << endl;
		return 1;
	}
	catch (const mysqlpp::Exception& er) {
		// Catch-all for any other MySQL++ exceptions
		cerr << endl << "Error: " << er.what() << endl;
		return 1;
	}
}

int main()
{
	//install();
	//test_mycache();
	test_mysql();
	return 0;
}