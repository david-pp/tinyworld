#include "mycached.h"
#include <string>
#include <queue>
#include <iostream>

#include "mylogger.h"
#include "mycache.h"
#include "mycache.pb.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <yaml-cpp/yaml.h>

static LoggerPtr logger(Logger::getLogger("mycache"));

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

#define safe_delete(ptr) if(ptr) { delete ptr; ptr = NULL; }

static YAML::Node config;
static int s_tablenum = 10;
static int s_cacheonly = 0;

///////////////////////////////////////////////////////////////////////

static std::string getKVSTableName(const std::string& key)
{
	static char name[255] = "";

	uint32_t hash = hash_murmur(key);
	snprintf(name, sizeof(name), "kvs_%u", (hash%s_tablenum));
	return name;
}

struct GetMsgHandler : public MsgHandler
{
	bool doMsg(std::string& reply, const std::string& msg)
	{
		std::string key = msg;

		RedisConnectionByKey redis(key, worker->cachePool());
		if (redis)
		{
			RedisReply::Map kvs;
			if (redis->hgetall(key, kvs))
			{
				reply = kvs["value"];
				LOG4CXX_INFO(logger, "get " << key << " .. hit");
				return true;
			}
		}
		else
		{
			LOG4CXX_WARN(logger, "get " << key << " .. redis is not ready");
		}

		if (0 == s_cacheonly)
		{
			MySqlConnectionByKey mysql(key, worker->dbPool());
			if (mysql)
			{
				try {
					mysqlpp::Query query = mysql->query();
					query << "select * from "<< getKVSTableName(key) << " where `key`=" << mysqlpp::quote << key;
					mysqlpp::StoreQueryResult res = query.store();
					if (res)
					{
						if (res.num_rows() == 1)
						{
							reply.resize(res[0]["value"].size());
							reply.assign(res[0]["value"].data(), res[0]["value"].size());

							redis->hmset("%b value %b flag %u touchtime %u", 
								key.data(), key.size(),
								reply.data(), reply.size(), 0, (unsigned int)time(0));

							LOG4CXX_INFO(logger, "get " << key << " .. miss");
							return true;
						}
					}
				}
				catch (std::exception& err)
				{
					LOG4CXX_WARN(logger, "get " << key << " .. mysql error:" << err.what());
					return false;
				}
			}
			else
			{
				LOG4CXX_WARN(logger, "get " << key << " .. mysql is not ready");
			}
		}

		reply = "";
		LOG4CXX_INFO(logger, "get " << key << " .. non-exists");
		return true;
	}
};

struct SetMsgHandler : public MsgHandler
{
	bool doMsg(std::string& reply, const std::string& msg)
	{
		Cmd::Set setmsg;
		if (setmsg.ParseFromString(msg))
		{
			const std::string& key = setmsg.key();
			const std::string& value = setmsg.value();

			LOG4CXX_INFO(logger, "<" << pthread_self() << "> set : " << setmsg.key() << ", " << setmsg.value().size());


			RedisConnectionByKey redis(key, worker->cachePool());
			if (redis)
			{
				redis->hmset("%s value %b flag %u touchtime %u", key.c_str(),
						value.data(), value.size(), 0, (unsigned int)time(0));
			}
			else
			{
				LOG4CXX_WARN(logger, "set " << key << " .. redis is not ready");
			}

			if (0 == s_cacheonly)
			{
				MySqlConnectionByKey mysql(key, worker->dbPool());
				if (mysql)
				{
					try {
						mysqlpp::Query query = mysql->query();
						query.reset();
						query << "replace into " 
						      << getKVSTableName(key) 
						      << "(`key`, value, flag, touchtime) values(" 
							  << mysqlpp::quote << key << ","
							  << mysqlpp::quote << value << ","
							  << 0 << ","
							  << time(0) << ")";
						query.execute();

						LOG4CXX_INFO(logger, "mysql " << key << "," << mysql->shard() << "," << getKVSTableName(key));
					}
					catch (std::exception& err)
					{
						LOG4CXX_WARN(logger, "set " << key << " .. mysql error:" << err.what());
						return false;
					}
				}
				else
				{
					LOG4CXX_WARN(logger, "set " << key << " .. mysql is not ready");
				}
			}
		}

		
		reply = "OK";
		return true;
	}
};


struct DelMsgHandler : public MsgHandler
{
	bool doMsg(std::string& reply, const std::string& msg)
	{
		const std::string& key = msg;

		RedisConnectionByKey redis(key, worker->cachePool());
		if (redis)
		{
			redis->del(key);
			LOG4CXX_INFO(logger, "del " << key);
		}
		else
		{
			LOG4CXX_WARN(logger, "del " << key << " .. redis is not ready");
		}


		if (0 == s_cacheonly)
		{
			MySqlConnectionByKey mysql(key, worker->dbPool());
			if (mysql)
			{
				try {
					mysqlpp::Query query = mysql->query();
					query.reset();
					query << "delete from " << getKVSTableName(key) << " where `key`=" 
						  << mysqlpp::quote << key;
					query.execute();

					LOG4CXX_INFO(logger, "mysql: del" << key << "," << mysql->shard() << "," << getKVSTableName(key));

					reply = "OK";
				}
				catch (std::exception& err)
				{
					LOG4CXX_WARN(logger, "del " << key << " .. mysql error:" << err.what());
					return false;
				}
			}
			else
			{
				LOG4CXX_WARN(logger, "del " << key << " .. mysql is not ready");
			}
		}

		return true;
	}
};

MsgHandler* MsgHandler::createByType(unsigned char cmdtype)
{
	switch (cmdtype)
	{
		case kMyCacheCmd_Get: return new GetMsgHandler;
		case kMyCacheCmd_Set: return new SetMsgHandler;
		case kMyCacheCmd_Del: return new DelMsgHandler;
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////



MyCacheWorker::MyCacheWorker(zmq::context_t* context, const std::string& address, int id)
{
	context_ = context;
	address_ = address;
	socket_ = NULL;
	cachepool_ = NULL;
	dbpool_ = NULL;
	id_  = id;

	std::ostringstream oss;
	oss << "w" << id_;
	name_ = oss.str();
}

MyCacheWorker::~MyCacheWorker()
{
	reset();
}

void MyCacheWorker::reset()
{
	safe_delete(socket_);
	safe_delete(cachepool_);
	safe_delete(dbpool_);
}

bool MyCacheWorker::init()
{
	if (!connect())
		return false;

    if (!initRedis())
    	return false;

    if (!initMySQL())
    	return false;

    return true;
}

bool MyCacheWorker::connect()
{
	if (socket_)
	{
		LOG4CXX_ERROR(logger, "worker connect failed : already exists");
		return false;
	}

	try 
	{
 		socket_ = new zmq::socket_t (*context_, ZMQ_REQ);
    	if (socket_)
    	{
			socket_->setsockopt (ZMQ_IDENTITY, name_.data(), name_.size());
    		socket_->connect (address_.c_str());
    		LOG4CXX_INFO(logger, "worker connected :" << address_);
    		return true;
    	}
    }
    catch (std::exception& err)
    {
    	LOG4CXX_ERROR(logger, "worker connect failed : " << err.what());
    }

    return false;
}

bool MyCacheWorker::initRedis()
{
	cachepool_ = new RedisShardingPool();
	if (cachepool_)
	{
		cachepool_->setShardNum(config["caches"].size());

		for (YAML::const_iterator it = config["caches"].begin(); 
			it != config["caches"].end(); ++it)
		{
			cachepool_->addSharding(it->as<std::string>());
		}

		if (!cachepool_->isReady())
    	{
    		LOG4CXX_ERROR(logger, "redis is not ready! " << cachepool_->shardNum());
    		return false;
    	}

    	cachepool_->init();

    	LOG4CXX_INFO(logger, "redis init .. " << cachepool_->shardNum());
    	return true;
	}

	return false;
}

bool MyCacheWorker::initMySQL()
{
    dbpool_ = new MySqlShardingPool();
    if (dbpool_)
    {
    	dbpool_->setShardNum(config["databases"].size());

    	for (YAML::const_iterator it = config["databases"].begin(); 
			it != config["databases"].end(); ++it)
		{
			dbpool_->addSharding(it->as<std::string>());
		}

    	if (!dbpool_->isReady())
    	{
    		LOG4CXX_ERROR(logger, "mysql is not ready! " << dbpool_->shardNum());
    		return false;
    	}

    	cachepool_->init();
    	
    	LOG4CXX_INFO(logger, "mysql init .. " << dbpool_->shardNum());
    	return true;
    }

    return false;
}

void MyCacheWorker::run()
{
	LOG4CXX_INFO(logger, "[" << name_ << "] worker started");

	//  Tell backend we're ready for work
    socket_->send("READY", 5);

	while (!s_interrupted)
	{
		try 
		{
			//  Read and save all frames until we get an empty frame
	        //  In this example there is only 1 but it could be more
	       	zmq::message_t client_addr_msg;
	       	socket_->recv(&client_addr_msg);

	       	//  Empty Frame
	       	{
		       	zmq::message_t empty;
		       	socket_->recv(&empty);
	       	}
	  
	        //  Get request, send reply
	        zmq::message_t request;
	        socket_->recv(&request);

	        //std::cout << "Worker: " << (char*)request.data() << std::endl;

	        // Do the Request
	        std::string reply;
	    	uint64_t id = 0;
	    	uint8_t cmd = 0;
	    	std::string msgbody;
	    	std::string clientid((char*)client_addr_msg.data(), client_addr_msg.size());
	    	if (MyCache::parseBinMsg(request, id, cmd, msgbody))
	    	{
	    		#if 0
	    		LOG4CXX_INFO(logger, "[" << name_ << "] [" << clientid << "] " 
	    			<< (char)cmd << " : "
	    			<< id << " : " 
	    			<< msgbody.size());
	    		#endif

	    		MsgHandler* handler = MsgHandler::createByType(cmd);
				if (handler)
				{
					handler->worker = this;
					handler->doMsg(reply, msgbody);
					delete handler;
				}
	    	}
	    	else
	    	{
	    		LOG4CXX_WARN(logger, "[" << name_ << "] [" << clientid << "] " 
	    			<< (char)cmd << " : "
	    			<< id << " : parse msg failed");
	    	}

			//reply = "FUCKYOU";
			
			// Reply
			{
				zmq::message_t empty;
		        socket_->send(client_addr_msg, ZMQ_SNDMORE);
		        socket_->send(empty, ZMQ_SNDMORE);
		        
		        zmq::message_t replymsg;
		        MyCache::packBinMsg(replymsg, id, cmd, reply);
		        socket_->send(replymsg);
	    	}
		}
		catch(const std::exception& err)
		{
			LOG4CXX_ERROR(logger, "[" << name_ << "] worker error : " << err.what());
			break;
		}
	}

	LOG4CXX_INFO(logger, "[" << name_ << "] worker finished");

	delete this;
}

RedisShardingPool* MyCacheWorker::cachePool()
{
	return cachepool_ != NULL ? cachepool_ : RedisShardingPool::instance();
}

MySqlShardingPool* MyCacheWorker::dbPool()
{
	return dbpool_ != NULL ? dbpool_ : MySqlShardingPool::instance();
}

///////////////////////////////////////////////////////////////////

MyCacheServer::MyCacheServer()
{
	address_ = "tcp://*:5555";
	clients_socket_ = NULL;
	workers_socket_ = NULL;
	context_ = NULL;
	workernum_ = 3;
}

MyCacheServer* MyCacheServer::instance()
{
	static MyCacheServer server;
	return &server;
}

MyCacheServer::~MyCacheServer()
{
}

void MyCacheServer::fini()
{
	safe_delete(clients_socket_);
	safe_delete(workers_socket_);
	safe_delete(context_);

	worker_thrds.join_all();
}


bool MyCacheServer::loadConfig(const std::string& cfg, const std::string& servernode)
{
	try 
	{
		config = YAML::LoadFile(cfg)[servernode];

		address_ = config["listen"].as<std::string>();
		if (address_.empty())
		{
			LOG4CXX_ERROR(logger, "loadConfig failed: " << cfg << ": no listen");
			return false;
		}

		workernum_ = config["workers"].as<int>();
		if (workernum_ == 0)
		{
			workernum_ = 1;
		}

		if (config["caches"].size() == 0 || config["databases"].size() == 0)
		{
			LOG4CXX_ERROR(logger, "loadConfig failed: " << cfg << ": no caches/databases");
			return false;
		}

		s_tablenum = config["tablenums"].as<int>();
		if (s_tablenum == 0)
			s_tablenum = 10;

		s_cacheonly = config["cacheonly"].as<int>();

		return true;

	}
	catch (const std::exception& err)
	{
		LOG4CXX_ERROR(logger, "loadConfig failed: " << cfg << ": "<< err.what());
	}

	return false;
}

bool MyCacheServer::init()
{
	s_catch_signals();

	try 
	{
		context_ = new zmq::context_t(1);

		clients_socket_ = new zmq::socket_t(*context_, ZMQ_ROUTER);
		if (clients_socket_)
		{
			clients_socket_->bind(address_.c_str());
			LOG4CXX_INFO(logger, "init - bind client sockets sucess:" << address_);
		}

		workers_socket_ = new zmq::socket_t(*context_, ZMQ_ROUTER);
		if (workers_socket_)
		{
			workers_socket_->bind("inproc://workers");
			LOG4CXX_INFO(logger, "init - bind worker sockets sucess:inproc://workers");
		}
	}
	catch (std::exception& err)
	{
		LOG4CXX_ERROR(logger, "init - bind failed :" << err.what());
		return false;
	}

    //  Launch pool of worker threads
    for (int i = 0; i < workernum_; i++) 
    {
    	MyCacheWorker* worker = new MyCacheWorker(this->context_, "inproc://workers", i);
    	if (worker && worker->init())
    	{
    		worker_thrds.create_thread(boost::bind(&MyCacheWorker::run, worker));
    	}
    }

	return true;
}

void MyCacheServer::run()
{
	if (!clients_socket_ || !workers_socket_) 
		return;

	//  Logic of LRU loop
    //  - Poll backend always, frontend only if 1+ worker ready
    //  - If worker replies, queue worker as ready and forward reply
    //    to client if necessary
    //  - If client requests, pop next worker and send request to it
    //
    //  A very simple queue structure with known max size
    std::vector<std::string> worker_queue;
    size_t worker_queue_selector = 0;

    //  Initialize poll set
    zmq::pollitem_t items[] = {
        	//  Always poll for worker activity on backend
            { *workers_socket_, 0, ZMQ_POLLIN, 0 },
            //  Poll front-end only if we have available workers
            { *clients_socket_, 0, ZMQ_POLLIN, 0 }
    };

    while (!s_interrupted) 
    {
    	try 
    	{
	        if (worker_queue.size())
	            zmq::poll(&items[0], 2, 100);
	        else
	            zmq::poll(&items[0], 1, 100);

	        //  Handle worker activity on backend
	        if (items[0].revents & ZMQ_POLLIN) 
	        {
	            //  Queue worker address for LRU routing
	            zmq::message_t worker_addr;
	            workers_socket_->recv(&worker_addr);

	            //  Second frame is empty
	            {
		            zmq::message_t empty;
		            workers_socket_->recv(&empty);
	        	}
	      
	            //  Third frame is READY or else a client reply address
	            zmq::message_t client_addr_msg;
	            workers_socket_->recv(&client_addr_msg);
	            std::string client_addr((char*)client_addr_msg.data(), client_addr_msg.size());

	            //  If client reply, send rest back to frontend
	            if (client_addr.compare("READY") != 0) 
	            {
	            	// empty frame
	            	{
		            	zmq::message_t empty;
		            	workers_socket_->recv(&empty);
	            	}

	            	zmq::message_t reply;
	            	workers_socket_->recv(&reply);

	            	// std::string ret((char*)reply.data(), reply.size());
	            	// std::cout << "frontend->client:" << ret << std::endl;

	            	clients_socket_->send(client_addr_msg, ZMQ_SNDMORE);
	            	
	            	// Sync Client(REQ)
	            	if (client_addr[0] != '_')
	            	{
	            		zmq::message_t empty;
	            		clients_socket_->send(empty, ZMQ_SNDMORE);
	            	}

	            	clients_socket_->send(reply);
	            }
	            else
	            {
	            	worker_queue.push_back(std::string((char*)worker_addr.data(), worker_addr.size()));
	            }
	        }
	        if (items[1].revents & ZMQ_POLLIN) 
	        {

	            //  Now get next client request, route to LRU worker
	            //  Client request is [address][empty][request]
	            zmq::message_t client_addr_msg;
	            clients_socket_->recv(&client_addr_msg);
	            std::string id((char*)client_addr_msg.data(), client_addr_msg.size());

	            // Sync Client(REQ)
	            if (id[0] != '_')
	            {
	            	zmq::message_t empty;
	            	clients_socket_->recv(&empty);
	            }

	            zmq::message_t request;
	            clients_socket_->recv(&request);
	            
	            if (worker_queue.size() > 0)
	            {
		            std::string worker_addr = worker_queue[(worker_queue_selector++) % worker_queue.size()];

		            zmq::message_t empty;
		            workers_socket_->send(worker_addr.data(), worker_addr.size(), ZMQ_SNDMORE);
		           	workers_socket_->send(empty, ZMQ_SNDMORE);
		            workers_socket_->send(client_addr_msg, ZMQ_SNDMORE);
		            workers_socket_->send(empty, ZMQ_SNDMORE);
		            workers_socket_->send(request);
	        	}
	        }
    	}
    	catch (const std::exception& err)
    	{
    		LOG4CXX_ERROR(logger, "server run failed:" << err.what());
    		break;
    	}
    }

    LOG4CXX_INFO(logger, "server will be shutdown");

    fini();
}


#ifdef _ONLY_FOR_TEST

#include <stdlib.h>
#include <unistd.h>
#include "url.h"

void resetMySql(const std::string& mysqlurl, const std::string& dbname, int num = 10)
{
	MySqlConnection mysql;
	if (mysql.connectByURL(mysqlurl))
	{
		if (true)
		{
			mysqlpp::NoExceptions ne(mysql);
			mysqlpp::Query query = mysql.query();
			if (!mysql.select_db(dbname.c_str())) 
			{
				if (!mysql.create_db(dbname.c_str()) || !mysql.select_db(dbname.c_str()))
				{
					LOG4CXX_ERROR(logger, "reset mysql failed ..");
					return;
				}
			}
		}

		LOG4CXX_INFO(logger, "creating tables ...");

		try {
			mysqlpp::Query query = mysql.query();

			for (int i = 0; i < num; i++)
			{
				query.reset();
				query << "DROP TABLE IF EXISTS KVS_" << i;
				query.execute();

				query.reset();
				query << 
						"CREATE TABLE KVS_" << i << " (" <<
						"  `key` VARCHAR(255) NOT NULL, " <<
						"  value BLOB NOT NULL," <<
						"  flag INT(10) UNSIGNED NOT NULL, " <<
						"  touchtime INT(10) UNSIGNED NOT NULL, "<<
						"PRIMARY KEY (`key`))" <<
						"ENGINE = InnoDB " <<
						"CHARACTER SET utf8 COLLATE utf8_general_ci";
				query.execute();

				LOG4CXX_INFO(logger, "KVS_" << i);
			}
		}
		catch (std::exception& err)
		{
			LOG4CXX_ERROR(logger, "reset mysql failed:" << err.what());
		}
	}
}

int main(int argc, const char* argv[])
{
	std::fstream loggerconfig("logger.properties");
	if (loggerconfig.is_open())
	{
		loggerconfig.close();
		PropertyConfigurator::configure("logger.properties");
	}
	else
	{
		BasicConfigurator::configure();
	}

	if (argc > 1)
	{
		std::string arg1 = argv[1];
		if ("help" == arg1)
		{
			std::cout << "mycached help | resetmysql | .." << std::endl;
			return 0;
		}
		else if ("resetmysql" == arg1)
		{
			MyCacheServer::instance()->loadConfig();

			for (YAML::const_iterator it = config["databases"].begin(); it != config["databases"].end(); ++it)
			{
				URL url;
				if (url.parse(it->as<std::string>()))
				{
					resetMySql(it->as<std::string>(), url.path, config["tablenums"].as<int>());
				}
			}
		}

		return 0;
	}

	MyCacheServer::instance()->loadConfig();
	MyCacheServer::instance()->init();
	MyCacheServer::instance()->run();

	LOG4CXX_INFO(logger, "over...");
}

#endif
