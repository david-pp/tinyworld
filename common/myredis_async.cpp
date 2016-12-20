#include "myredis_async.h"
#include "mylogger.h"
#include "url.h"
#include "hiredis/adapters/libev.h"

static LoggerPtr logger(Logger::getLogger("myredis"));

struct SchedulerRun : public TimerEvent
{
public:
	SchedulerRun(AsyncRedisClient* c) : client(c) {}

	void called()
	{
		if (client)
			client->run();
	}

public:
	AsyncRedisClient* client;
};


AsyncRedisClient::AsyncRedisClient(const std::string& ip, int port, EventLoop* loop)
{
	context_ = NULL;
	ip_ = ip;
	port_ = port;
	db_ = 0;
	evloop_ = loop;
	runByIdle();
}

AsyncRedisClient::AsyncRedisClient(const std::string& urltext, EventLoop* loop)
{
	context_ = NULL;
	ip_ = "";
	port_ = 0;
	db_ = 0;
	evloop_ = loop;

	URL url;
	if (url.parse(urltext))
	{
		ip_ = url.host;
		port_ = url.port;
	}

	runByIdle();
}

AsyncRedisClient::~AsyncRedisClient()
{
	close();
}

void AsyncRedisClient::runByIdle()
{
	if (evloop_)
	{
		evloop_->regTimer(new SchedulerRun(this), 0.000050);
	}
}

static void connectCallback(const redisAsyncContext *c, int status) 
{
    AsyncRedisClient* client = (AsyncRedisClient*)c->data;
    if (client)
    	client->onConnected(status == REDIS_OK);
}

static void disconnectCallback(const redisAsyncContext *c, int status) 
{
    AsyncRedisClient* client = (AsyncRedisClient*)c->data;
    if (client)
    	client->onDisconnected(status == REDIS_OK);
}

void AsyncRedisClient::onConnected(bool success)
{
	if (success)
	{
		LOG4CXX_INFO(logger, "redis://" << ip_ << ":" << port_ << "/" << db_ << " async connected success");
	}
	else
	{
		LOG4CXX_ERROR(logger, "redis://" << ip_ << ":" << port_ << " async connected failed:" << context_->errstr);
		context_ = NULL;
	}
}

void AsyncRedisClient::onDisconnected(bool success)
{
	context_ = NULL;
	LOG4CXX_INFO(logger, "redis://" << ip_ << ":" << port_ << " async disconnected");
}

bool AsyncRedisClient::connect()
{
	if (!evloop_)
	{
		LOG4CXX_ERROR(logger, "redis://" << ip_ << ":" << port_ << 
			" Connectin error: event loop is not ready");
		return false;
	}

	context_ = redisAsyncConnect(ip_.c_str(), port_);
	if (context_ == NULL)
	{
		LOG4CXX_ERROR(logger, "redis://" << ip_ << ":" << port_ << 
			" Connection error: can't allocate redis context");
		return false;
	}

    if (context_->err) 
    {
    	LOG4CXX_ERROR(logger, "redis://" << ip_ << ":" << port_ << 
    		" Connection error: " <<context_->errstr);
    	close();
    	return false;
    }

    context_->data = this;

	redisLibevAttach(evloop_->evLoop(), context_);
	redisAsyncSetConnectCallback(context_, connectCallback);
	redisAsyncSetDisconnectCallback(context_, disconnectCallback);
	return true;
}


void AsyncRedisClient::run()
{
	scheduler_.run();
}

void AsyncRedisClient::close()
{
	if (context_) 
	{
		redisAsyncDisconnect(context_);
		context_ = NULL;
	} 
}

bool AsyncRedisClient::command(myredis::RPC* callback, const char* format, ...)
{
	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = command_v(callback, format, ap);
	va_end(ap);
	return ret;
}

struct CommandCallBackArg
{
	AsyncRPCScheduler* scheduler;
	uint64_t rpcid;
};

// r -> redisReply
static void cmdCallback(redisAsyncContext *c, void *r, void *privdata) 
{
	CommandCallBackArg* arg = (CommandCallBackArg*)privdata;
	if (arg)
	{
		arg->scheduler->triggerCall(arg->rpcid, r);
		delete arg;
	}
}

bool AsyncRedisClient::command_v(myredis::RPC* callback, const char* format, va_list ap)
{
	if (!callback) return false;

	checkConnection();
	if (!isConnected())
		return false;

	callback->client = this;

	CommandCallBackArg* arg = new CommandCallBackArg;
	arg->scheduler = &this->scheduler_;
	arg->rpcid = callback->id;

	redisvAsyncCommand(context_, cmdCallback, arg, format, ap);
    return true;
}


bool AsyncRedisClient::command_argv(myredis::RPC* callback, const std::vector<std::string>& args)
{
	if (!callback) return false;

	checkConnection();
	if (!isConnected())
		return false;

	callback->client = this;

	CommandCallBackArg* arg = new CommandCallBackArg;
	arg->scheduler = &this->scheduler_;
	arg->rpcid = callback->id;

	int argc = (int)args.size();
	char** argv = new char* [args.size()];
	size_t* argvlen = new size_t [args.size()];
	for (size_t i = 0; i < args.size(); i++)
	{
		argv[i] = (char*)args[i].data();
		argvlen[i] = args[i].size();
	}

	redisAsyncCommandArgv(context_, cmdCallback, arg, argc, (const char**)argv, argvlen);

    delete [] argv;
	delete [] argvlen;
    return true;
}


void AsyncRedisClient::checkConnection()
{
	if (!context_)
		connect();
}

bool AsyncRedisClient::call(myredis::RPC* rpc, const ContextPtr& context, int timeoutms)
{
	if (!rpc) return false;

	rpc->client = this;
	scheduler_.call(rpc, context, timeoutms);
	return true;
}

bool AsyncRedisClient::callBySerials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (cb)
	{
		for (size_t i = 0; i < rpcs.size(); i++)
		{
			myredis::RPC* child = (myredis::RPC*)rpcs[i];
			child->client = this;
		}

		return scheduler_.callBySerials(cb, rpcs, context, timeoutms);
	}
	return false;
}

bool AsyncRedisClient::callByParallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (cb)
	{
		for (size_t i = 0; i < rpcs.size(); i++)
		{
			myredis::RPC* child = (myredis::RPC*)rpcs[i];
			child->client = this;
		}
		return scheduler_.callByParallel(cb, rpcs, context, timeoutms);
	}
	return false;
}


static void cmdCallback2(redisAsyncContext* c, void* r, void* privdata)
{
	myredis::Callback* cb = (myredis::Callback*)privdata;
	if (cb)
	{
		RedisReply reply;
		reply.parseFrom((redisReply*)r);
		cb->replied(reply);
		delete cb;
	}
}

bool AsyncRedisClient::cmd(myredis::Callback* cb, const char* format, ...)
{
	bool ret = false;
	va_list ap;
	va_start(ap, format);
	ret = cmd_v(cb, format, ap);
	va_end(ap);
	return ret;
}

bool AsyncRedisClient::cmd_v(myredis::Callback* cb, const char* format, va_list ap)
{
	if (!cb) return false;

	checkConnection();
	if (!isConnected())
		return false;

	if (cb)
	{
		cb->client = this;
	}

	redisvAsyncCommand(context_, cmdCallback2, cb, format, ap);
}

bool AsyncRedisClient::cmd_argv(myredis::Callback* cb, const std::vector<std::string>& args)
{
	if (!cb) return false;

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

	redisAsyncCommandArgv(context_, cmdCallback2, cb, argc, (const char**)argv, argvlen);

    delete [] argv;
	delete [] argvlen;

	return true;
}
