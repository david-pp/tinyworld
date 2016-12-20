#ifndef __COMMON_MYCACHED_H
#define __COMMON_MYCACHED_H

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include "zmq.hpp"
#include "mydb.h"
#include "myredis_pool.h"
#include "hashkit.h"

class MyCacheWorker;

struct MsgHandler
{
	MsgHandler() { worker = NULL; }

	static MsgHandler* createByType(unsigned char cmdtype);
	virtual bool doMsg(std::string& reply, const std::string& request) = 0;

	MyCacheWorker* worker;
};

class MyCacheWorker
{
public:
	MyCacheWorker(zmq::context_t* context, const std::string& address, int id);
	~MyCacheWorker();

	bool init();
	void reset();
	void run();

	RedisShardingPool* cachePool();
	MySqlShardingPool* dbPool();

protected:
	bool connect();
	bool initRedis();
	bool initMySQL();

private:
	int id_;
	std::string name_;

	zmq::context_t* context_;
	std::string     address_;
	zmq::socket_t*  socket_;

	RedisShardingPool* cachepool_;
	MySqlShardingPool* dbpool_;
};

class MyCacheServer
{
public:
	MyCacheServer();
	~MyCacheServer();

	static MyCacheServer* instance();

	bool loadConfig(const std::string& cfg = "config.yaml", const std::string& servernode="default");

	bool init();
	void run();
	void fini();

	zmq::context_t* context() { return context_; }

private:
	zmq::context_t* context_;
	zmq::socket_t*  clients_socket_;
	zmq::socket_t*  workers_socket_;

	int workernum_;

	std::string address_;

	std::vector<MyCacheWorker*> workers_;

	boost::thread_group worker_thrds;
};




#endif // __COMMON_MYCACHED_H