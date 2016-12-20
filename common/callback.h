#ifndef __COMMON_CALLBACK_H
#define __COMMON_CALLBACK_H

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <cstdarg>
#include <boost/thread.hpp>
#include <boost/any.hpp>
#include <boost/date_time.hpp>
#include <boost/smart_ptr.hpp>

struct Context : public std::map<std::string, boost::any> 
{
	typedef std::map<std::string, boost::any> Base;

	~Context()
	{
		//std::cout << "context over: " << debugString() << std::endl;
	}

	template <typename T>
	void set(const std::string& key, T value)
	{
		(*this)[key] = value;
	}

	template <typename T>
	T get(const std::string& key)
	{
		typename Base::iterator it = this->find(key);
		if (it == this->end())
			return T();

		try
	    {
	        return boost::any_cast<T>(it->second);
	    }
	    catch(const boost::bad_any_cast &)
	    {
	        return T();
	    }
	}

	std::string debugString()
	{
		std::ostringstream oss;
		for (Base::iterator it = this->begin(); it != this->end(); ++ it)
			oss << it->first << ",";
		return oss.str();
	}
};

typedef boost::shared_ptr<Context> ContextPtr;

///////////////////////////////////////////////////////////

class AsyncRPCScheduler;

class AsyncRPC
{
public:
	typedef boost::shared_ptr<AsyncRPC> SharedPtr;
	typedef std::map<uint64_t, SharedPtr> Map;
	typedef std::vector<AsyncRPC*> PtrArray;

	static PtrArray S(AsyncRPC* rpc1, ...);
	static PtrArray S_v(AsyncRPC* rpc1, va_list ap);

public:
	AsyncRPC();
	AsyncRPC(const PtrArray& rpcs);
	
	virtual ~AsyncRPC();

	bool isTimeout();
	void setChildren(const PtrArray& rpcs);

public:
	virtual void request() {}

	virtual void called(void* data) = 0;
	virtual void timeouted() {}
	virtual void canceled();

	virtual void childCalled(const SharedPtr& child) {}
	virtual void childTimeouted(const SharedPtr& child) {}

public:
	// identifier
	uint64_t id;
	
	// for timeout 
	uint32_t timeoutms;
	boost::posix_time::ptime createtime;

	// contex for sharing variables
	ContextPtr context;

	// for hierachy
	AsyncRPC* parent;
	Map children;

	// scheduler
	AsyncRPCScheduler* scheduler;
};


class SerialsCallBack : public AsyncRPC
{
public:
	virtual ~SerialsCallBack()
	{
		std::cout << "serial : --- over" << std::endl;
	}

	virtual void called(void* data) = 0;

public:
	virtual void request();
	virtual void timeouted();

	virtual void childCalled(const SharedPtr& child);
	virtual void childTimeouted(const SharedPtr& child);

protected:
	void triggerFirstRequest();
};

class ParallelCallBack : public AsyncRPC
{
public:
	virtual ~ParallelCallBack()
	{
		std::cout << "serial : --- over" << std::endl;
	}

	virtual void called(void* data) = 0;

public:
	virtual void request();
	virtual void timeouted();

	virtual void childCalled(const SharedPtr& child);
	virtual void childTimeouted(const SharedPtr& child);
};


///////////////////////////////////////////////////
//
// Scheduler:
// 
//   thread1  thread2  thread3
//      |        |        |   - call
//      V        V        V
//     RRRRRRRRRRRRRRRRRRRRR  - ready queue
//               |
//  +-----------thread:run -------------------+
//  |            |  - request                 |
//  |            V                            |
//  |    WWWWWWWWWWWWWWWWWW  - waiting queue  |
//  |       | -reply  | -timeout              |
//  |       v         v                       | 
//  |   triggerCall  triggerTimeout           |
//  |                                         |
//  +-----------------------------------------+
//
///////////////////////////////////////////////////
class AsyncRPCScheduler
{
public:
	AsyncRPCScheduler() {}
	virtual ~AsyncRPCScheduler() {}

	bool call(AsyncRPC* rpc, const ContextPtr& context, int timeoutms = -1);
	bool callBySerials(SerialsCallBack* rpc, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);
	bool callByParallel(ParallelCallBack* rpc, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms = -1);

	void run();

	struct Stat
	{
		Stat();
		std::string debugString() const;

		long unsigned int rpc_construct;
		long unsigned int rpc_destroyed;

		long unsigned int rpc_requested;
		long unsigned int rpc_called;
		long unsigned int rpc_timeouted;
		long unsigned int rpc_canceled;

		long unsigned int queue_ready;
		long unsigned int queue_wait;
		long unsigned int wait_by_time;
	};

	Stat& stat();

public:
	void triggerRequest(const AsyncRPC::SharedPtr& rpc);

	void triggerCall(uint64_t id, void* data);
	void triggerCall(const AsyncRPC::SharedPtr& rpc, void* data);
	void triggerTimeout(const AsyncRPC::SharedPtr& rpc);
	void triggerCancel(const AsyncRPC::SharedPtr& rpc);

protected:
	typedef std::multimap<boost::posix_time::ptime, uint64_t> TimeIndex;

	// ready queue
	boost::mutex mutex_rq_;
	AsyncRPC::Map queue_ready_;

	AsyncRPC::Map queue_wait_;
	TimeIndex wait_by_time_;

	// statistics
	Stat stat_;
};


#endif // __COMMON_CALLBACK_H