
#include <iostream>
#include <sstream>
#include <cstdio>

#include <string>
#include <vector>


#include "../common/myredis.h"

RedisClient cli("127.0.0.1", 6379);

void test_command()
{
	RedisReply reply;
	cli.command(reply, "SET name %s", "david");
	std::cout << reply.debugString() << std::endl;

	cli.command(reply, "GET name");
	std::cout << reply.debugString() << std::endl;

	cli.command(reply, "DEL array");
	for (int i = 0; i < 10; ++ i)
	{
		cli.command(reply, "LPUSH array %d", i);
		std::cout << reply.debugString() << std::endl;
	}

	cli.command(reply, "LRANGE array 0 -1");
	std::cout << reply.debugString() << std::endl;
}

void test_pipeline()
{
	const int maxcount = 10;

	RedisReply reply;
	
	for (int i = 0; i < maxcount; ++ i)
		cli.appendCommand("GET name");

	for (int i = 0; i < maxcount; ++ i)
	{
		cli.getReply(reply);
		std::cout << reply.debugString() << std::endl;
	}

}

void test_pub()
{
	while (true)
	{
		sleep(1);
		std::cout << cli.publish("mychannel", "hello, how are you ? 测试一下") << std::endl;
	}
}

void test_sub()
{
	cli.subscribe("mychannel");

	RedisReply reply;
	while (cli.getReply(reply))
	{
		std::cout << reply.debugString() << std::endl;
	}
}

void test_psub()
{
	cli.psubscribe("%s %s", "mychannel", "test.*");
	//cli.psubscribe("mychannel test.*");

	RedisReply reply;
	while (cli.getReply(reply))
	{
		std::cout << reply.debugString() << std::endl;
	}
}

void test_hash()
{
	RedisReply reply;
	cli.command(reply, "HGETALL %s", "user");
	std::cout << reply.debugString() << std::endl;

	for (int i = 0; i < 10; i++)
	{
		std::ostringstream key;
		key << "u:" << i;

		std::ostringstream name;
		name << "david" << i;

		cli.command(reply, "HMSET %s:%d name %s age %d", 
			"u", i, name.str().c_str(), i);
		std::cout << reply.debugString() << std::endl;
	}

	cli.command(reply, "HGETALL u:2");
	std::cout << reply.debugString() << std::endl;
	
	RedisReply::Map user;
	if (cli.hgetall("u:2", user))
	{
		std::cout << "name:" << user["name"] << std::endl;
		std::cout << "age:" << user["age"] << std::endl;
	}
	else
	{
		std::cout << "not exists!!" << std::endl;
	}

	cli.hmset("%s name %s flag %d", "user", "david", 2);

}

int main(int argc, const char* argv[])
{
	if (argc > 1)
	{
		std::string argv1 = std::string(argv[1]);
		if (argv1 == "pub")
			test_pub();
		else if (argv1 == "sub")
			test_sub();
		else if (argv1 == "psub")
			test_psub();
		else if (argv1 == "hash")
			test_hash();
		return 0;
	}

	test_command();
	test_pipeline();
}