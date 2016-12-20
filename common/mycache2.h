#ifndef __COMMON_MYCACHE2_H
#define __COMMON_MYCACHE2_H

#include <string>
#include <map>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <boost/any.hpp>
#include "myredis.h"
#include "myredis_pool.h"

class SyncMyCache;
class AsyncMyCache;

////////////////////////////////////////////////////////////
//
// mycache API
//
////////////////////////////////////////////////////////////

namespace mycache 
{
	// cachel level
	enum LV
	{
		LV_INPROC = 0,  // L0 (in memory of current process)
		LV_LOCAL  = 1,  // L1
		LV_GLOBAL = 2,  // L2
	};

	typedef std::map<std::string, std::string> KVMap;
	
	// synchronous mode
	extern SyncMyCache* sync;

	// asynchronous mode 
	extern AsyncMyCache* async;

	// asynchonous callback base class
	template <typename ValueT>
	struct Get : public myredis::HGetAll
	{
	public:
		virtual void replied_as_kvs(RedisReply::Map& kvs)
		{
			// hit
			if (kvs.size() > 0)
			{
				ValueT value;
				if (value.parseFromKVMap(kvs))
					this->result(&value);
				else
					this->result(NULL);
			}
			// miss
			else
			{
				this->result(NULL);
			}
		}

		virtual void result(const ValueT* value) = 0;
	};

	template <typename ValueT>
	struct Set : public myredis::HMSet
	{
		void setKVS(const std::string& key, const RedisReply::Map& kvs)
		{
			key_ = key;
			kvs_ = kvs;
		}

		virtual void replied(RedisReply& reply)
		{
			result(reply.getStr() == "OK");
		}

		virtual void result(bool okay) {}
	};

	struct Del : public myredis::Del
	{
		void setKey(const std::string& key)
		{
			key_ = key;
		}

		virtual void replied(RedisReply& reply)
		{
			result(reply.getInteger() > 0);
		}

		virtual void result(bool okay) {}
	};
}


////////////////////////////////////////////////////////////
//
// sync mode
//
////////////////////////////////////////////////////////////
class SyncMyCache
{
public:
	typedef std::map<int, RedisConnectionPool*> Connections;
	typedef boost::unordered_map<std::string, boost::any> L0Map;

	static SyncMyCache& instance()
	{
		static SyncMyCache sync_mycache;
		return sync_mycache;
	}

public:
	RedisConnectionPool* client(int level);
	bool connect(int level, const std::string& address);
	void close();
	void dump_inproc();

public:
	//
	// From/To specified cache
	//
	template<typename ValueT>
	bool get_from(const std::string& key, ValueT& value, int level);
	
	template<typename ValueT>
	bool set_to(const std::string& key, const ValueT& value, int level);	

	bool del_from(const std::string& key, int level);
	

	//
	// Get by hit/miss. (L0 > L1 > L2 ...)
	//
	template <typename ValueT>
	bool get(const std::string& key, ValueT& value, int maxlevel = -1);
	template <typename ValueT>
	bool get_and_cache(const std::string& key, ValueT& value, int cachelv = 0, int maxlevel = -1);
	
	//
	// Set to spectified cache
	// 
	template<typename ValueT>
	bool set(const std::string& key, const ValueT& value, int level);
	
	//
	// Delete from caches whoese level is lower than maxlevel
	//
	bool del(const std::string& key, int maxlevel = -1);
	
private:
	bool get_from_remote(mycache::KVMap& kvs, const std::string& key, int level);
	bool set_to_remote(const mycache::KVMap& kvs, const std::string& key, int level);
	bool del_from_remote(const std::string& key, int level);

	// L0
	L0Map cache_;
	boost::mutex cache_mutex_;

	// L1,L2,...
	Connections clients_;
	boost::mutex mutex_;
};


////////////////////////////////////////////////////////////
//
// async mode
//
////////////////////////////////////////////////////////////
class AsyncMyCache
{
public:
	typedef std::map<int, AsyncRedisClient*> Connections;
	typedef boost::unordered_map<std::string, boost::any> L0Map;

	static AsyncMyCache& instance()
	{
		static AsyncMyCache async_mycache;
		return async_mycache;
	}

	AsyncRedisClient* client(int level);
	bool connect(int level, const std::string& address, EventLoop* eventloop = EventLoop::instance());
	void close();

public:
	//
	// From/To specified cache
	//
	template <typename ValueT>
	bool get_from(mycache::Get<ValueT>* cb, const std::string& key, int level);

	template<typename ValueT>
	bool set_to(mycache::Set<ValueT>* cb, const std::string& key, const ValueT& value, int level);

	bool del_from(mycache::Del* cb, const std::string& key, int level);

	//
	// Get by hit/miss. (L0 > L1 > L2 ...)
	//
	template <typename ValueT>
	bool get(mycache::Get<ValueT>* cb, const std::string& key, int maxlevel = -1, int cacheit = -1);
	template <typename ValueT>
	bool get_and_cache(mycache::Get<ValueT>* cb, const std::string& key, int cachelv = 0, int maxlevel = -1);

	//
	// Set to spectified cache
	// 
	template<typename ValueT>
	bool set(mycache::Set<ValueT>* cb, const std::string& key, const ValueT& value, int level);

	//
	// Delete from caches whoese level is lower than maxlevel
	//
	bool del(const std::string& key, int maxlevel = -1);
	
private:
	// L1,L2,...
	Connections clients_;
	boost::mutex mutex_;
};

#include "mycache2.in.h"

#endif