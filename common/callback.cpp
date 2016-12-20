#include "callback.h"

static uint64_t s_total_rpc = 0;
static boost::mutex s_mutex;

static uint64_t generateID()
{
	boost::lock_guard<boost::mutex> guard(s_mutex);
	return ++ s_total_rpc;
}

static uint64_t s_stat_asyncrpc_construct = 0;
static uint64_t s_stat_asyncrpc_destroyed = 0;


//////////////////////////////////////////////////////////////

AsyncRPC::AsyncRPC()
{
	id = generateID();
	createtime = boost::posix_time::microsec_clock::local_time();
	timeoutms = (uint32_t)-1;
	scheduler = NULL;
	parent = NULL;

	s_stat_asyncrpc_construct ++ ;
}

AsyncRPC::AsyncRPC(const PtrArray& rpcs)
{
	id = generateID();
	createtime = boost::posix_time::microsec_clock::local_time();
	timeoutms = (uint32_t)-1;
	scheduler = NULL;

	setChildren(rpcs);

	s_stat_asyncrpc_construct ++ ;
}

AsyncRPC::~AsyncRPC()
{
	s_stat_asyncrpc_destroyed ++;
}

bool AsyncRPC::isTimeout()
{
	return (boost::posix_time::microsec_clock::local_time() - createtime).total_milliseconds() >= timeoutms;
}

void AsyncRPC::setChildren(const PtrArray& rpcs)
{
	for (size_t i = 0; i < rpcs.size(); i++)
	{
		if (!rpcs[i]) continue;

		rpcs[i]->id = generateID();
		rpcs[i]->scheduler = this->scheduler;
		rpcs[i]->context = this->context;

		SharedPtr rpc_ptr(rpcs[i]);
		if (rpc_ptr)
		{
			this->children.insert(std::make_pair(rpc_ptr->id, rpc_ptr));
			rpc_ptr->parent = this;		
		}
	}
}

AsyncRPC::PtrArray AsyncRPC::S(AsyncRPC* rpc1, ...)  
{
	AsyncRPC::PtrArray rpcs;
	AsyncRPC* last = rpc1;

	va_list ap;
	va_start(ap, rpc1);

	while (last != NULL)
	{
		rpcs.push_back(last);
		last = va_arg(ap, AsyncRPC*);
	}

	va_end(ap);
	return rpcs;
} 

AsyncRPC::PtrArray AsyncRPC::S_v(AsyncRPC* rpc1, va_list ap)  
{
	AsyncRPC::PtrArray rpcs;
	AsyncRPC* last = rpc1;

	while (last != NULL)
	{
		rpcs.push_back(last);
		last = va_arg(ap, AsyncRPC*);
	}

	return rpcs;
} 

void AsyncRPC::canceled()
{
	for (Map::iterator it = children.begin(); it != children.end(); ++it)
		scheduler->triggerCancel(it->second);

	children.clear();
}

//////////////////////////////////////////////////////////////

void SerialsCallBack::triggerFirstRequest()
{
	Map::iterator first = children.begin();
	if (first != children.end())
	{
		scheduler->triggerRequest(first->second);
	}
}

void SerialsCallBack::request()
{
	//std::cout << "serial:  " << " -- request"  << children.size() << std::endl;

	triggerFirstRequest();
}

void SerialsCallBack::timeouted()
{
	//std::cout << "serial:  " << " -- timeout" << std::endl;

	AsyncRPC::canceled();	
}

void SerialsCallBack::childCalled(const SharedPtr& child)
{
	Map::iterator first = children.begin();
	if (first != children.end() && first->second->id == child->id)
	{
		children.erase(first);
		triggerFirstRequest();
	}

	if (children.empty())
	{
		scheduler->triggerCall(this->id, NULL);
	}
}

void SerialsCallBack::childTimeouted(const SharedPtr& child)
{
	// this will timeout next tick
	timeoutms = 0;
}

//////////////////////////////////////////////////////////////

void ParallelCallBack::request()
{
	//std::cout << "parallel:  " << " -- request" << std::endl;

	for (Map::iterator it = children.begin(); it != children.end(); ++ it)
	{
		scheduler->triggerRequest(it->second);
	}
}

void ParallelCallBack::timeouted()
{
	//std::cout << "parallel:  " << " -- timeout" << std::endl;

	AsyncRPC::canceled();	
}

void ParallelCallBack::childCalled(const SharedPtr& child)
{
	Map::iterator it = children.find(child->id);
	if (it != children.end())
	{
		children.erase(it);
	}

	if (children.empty())
	{
		scheduler->triggerCall(this->id, NULL);
	}
}

void ParallelCallBack::childTimeouted(const SharedPtr& child)
{
	// this will timeout next tick
	timeoutms = 0;
}

//////////////////////////////////////////////////////////////


bool AsyncRPCScheduler::call(AsyncRPC* rpc, const ContextPtr& context, int timeoutms)
{
	if (!rpc) return false;

	rpc->scheduler = this;
	rpc->context = context;
	if (timeoutms > 0)
		rpc->timeoutms = static_cast<uint32_t>(timeoutms);

	boost::lock_guard<boost::mutex> guard(mutex_rq_);
	AsyncRPC::SharedPtr rpc_ptr(rpc);
	queue_ready_.insert(std::make_pair(rpc->id, rpc_ptr));

	return true;
}

bool AsyncRPCScheduler::callBySerials(SerialsCallBack* rpc, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (!rpc) return false;

	rpc->scheduler = this;
	rpc->context = context;
	if (timeoutms > 0)
		rpc->timeoutms = static_cast<uint32_t>(timeoutms);

	rpc->setChildren(rpcs);

	boost::lock_guard<boost::mutex> guard(mutex_rq_);
	AsyncRPC::SharedPtr rpc_ptr(rpc);
	queue_ready_.insert(std::make_pair(rpc->id, rpc_ptr));

	return true;
}

bool AsyncRPCScheduler::callByParallel(ParallelCallBack* rpc, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (!rpc) return false;

	rpc->scheduler = this;
	rpc->context = context;
	if (timeoutms > 0)
		rpc->timeoutms = static_cast<uint32_t>(timeoutms);

	rpc->setChildren(rpcs);

	boost::lock_guard<boost::mutex> guard(mutex_rq_);
	AsyncRPC::SharedPtr rpc_ptr(rpc);
	queue_ready_.insert(std::make_pair(rpc->id, rpc_ptr));

	return true;
}

void AsyncRPCScheduler::run()
{
	stat_.queue_wait = queue_wait_.size();
	stat_.wait_by_time = wait_by_time_.size();

	// check timeout
	TimeIndex::iterator it = wait_by_time_.begin(), tmpit;
	while (it != wait_by_time_.end())
	{
		tmpit = it ++;

		AsyncRPC::Map::iterator rpcit = queue_wait_.find(tmpit->second);
		if (rpcit != queue_wait_.end())
		{
			if (rpcit->second->isTimeout())
			{
				triggerTimeout(rpcit->second);

				wait_by_time_.erase(tmpit);
			}
		}
		else
		{
			wait_by_time_.erase(tmpit);
		}
	}

	// trigger all request
	if (1)
	{
		boost::lock_guard<boost::mutex> guard(mutex_rq_);
		stat_.queue_ready = queue_ready_.size();
		for (AsyncRPC::Map::iterator it = queue_ready_.begin(); it != queue_ready_.end(); ++ it)
		{
			triggerRequest(it->second);
		}
		queue_ready_.clear();
	}
}

void AsyncRPCScheduler::triggerRequest(const AsyncRPC::SharedPtr& rpc)
{
	if (rpc)
	{
		//std::cout << "triggerRequest:" << rpc->id << std::endl;
		queue_wait_.insert(std::make_pair(rpc->id, rpc));
		if(rpc->timeoutms != (uint32_t)-1)
			wait_by_time_.insert(std::make_pair(rpc->createtime, rpc->id));
		rpc->request();	

		stat_.rpc_requested++;
	}
}

void AsyncRPCScheduler::triggerCall(uint64_t id, void* data)
{
	//std::cout << "triggerCall:" << id << std::endl;
	AsyncRPC::Map::iterator rpcit = queue_wait_.find(id);
	if (rpcit != queue_wait_.end())
	{
		triggerCall(rpcit->second, data);
	}
}

void AsyncRPCScheduler::triggerCall(const AsyncRPC::SharedPtr& rpc, void* data)
{
	if (rpc)
	{
		rpc->called(data);

		stat_.rpc_called ++;

		if (rpc->parent)
		{
			rpc->parent->childCalled(rpc);
		}

		queue_wait_.erase(rpc->id);
	}
}

void AsyncRPCScheduler::triggerTimeout(const AsyncRPC::SharedPtr& rpc)
{
	if (rpc)
	{
		rpc->timeouted();

		stat_.rpc_timeouted ++;

		if (rpc->parent)
		{
			rpc->parent->childTimeouted(rpc);
		}

		queue_wait_.erase(rpc->id);
	}
}

void AsyncRPCScheduler::triggerCancel(const AsyncRPC::SharedPtr& rpc)
{
	if (rpc)
	{
		rpc->canceled();
		stat_.rpc_canceled ++ ;
		queue_wait_.erase(rpc->id);
	}
}


/////////////////////////////////////////////////

AsyncRPCScheduler::Stat& AsyncRPCScheduler::stat()
{
	stat_.rpc_construct = s_stat_asyncrpc_construct;
	stat_.rpc_destroyed = s_stat_asyncrpc_destroyed;
	return stat_;
}

AsyncRPCScheduler::Stat::Stat()
{
	rpc_construct = 0;
	rpc_destroyed = 0;

	rpc_requested = 0;
	rpc_called = 0;
	rpc_timeouted = 0;
	rpc_canceled = 0;	

	queue_ready = 0;
	queue_wait = 0;
	wait_by_time = 0;
}

std::string AsyncRPCScheduler::Stat::debugString() const
{
	char text[256] = "";
	snprintf(text, sizeof(text), "queue:[r=%lu w=%lu w2=%lu], rpc:[c=%lu d=%lu], rpc:[r=%lu, c=%lu, t=%lu , c=%lu]",
		queue_ready, queue_wait, wait_by_time,
		rpc_construct, rpc_destroyed,
		rpc_requested, rpc_called, rpc_timeouted, rpc_canceled);

	return text;
}