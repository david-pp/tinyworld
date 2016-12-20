
template<typename ValueT>
inline bool SyncMyCache::get_from(const std::string& key, ValueT& value, int level)
{
	// in memory
	if (0 == level)
	{
		boost::lock_guard<boost::mutex> guard(cache_mutex_);
		L0Map::iterator it = cache_.find(key);
		if (it != cache_.end())
		{
			try
		    {
		    	value = boost::any_cast<ValueT>(it->second);
		        return true;
		    }
		    catch(const boost::bad_any_cast &)
		    {
		        return false;
		    }
		}
	}
	// in remote
	else
	{
		mycache::KVMap kvs;
		if (get_from_remote(kvs, key, level))
		{
			return value.parseFromKVMap(kvs);
		}
	}

	return false;
}

template<typename ValueT>
inline bool SyncMyCache::set_to(const std::string& key, const ValueT& value, int level)
{
	// to memory
	if (0 == level)
	{
		boost::lock_guard<boost::mutex> guard(cache_mutex_);
		cache_[key] = value;
	}
	// to remote
	else
	{
		mycache::KVMap kvs;
		if (value.saveToKVMap(kvs))
		{
			return set_to_remote(kvs, key, level);
		}
	}

	return false;
}

inline bool SyncMyCache::del_from(const std::string& key, int level)
{
	// from memory
	if (0 == level)
	{
		boost::lock_guard<boost::mutex> guard(cache_mutex_);
		L0Map::iterator it = cache_.find(key);
		if (it != cache_.end())
		{
			cache_.erase(it);
			return true;
		}
	}
	// from remote
	else
	{
		return del_from_remote(key, level);
	}

	return false;
}


template <typename ValueT>
inline bool SyncMyCache::get(const std::string& key, ValueT& value, int maxlevel /*= -1*/)
{
	const int maxlv = (maxlevel == -1 ? mycache::LV_GLOBAL : maxlevel);

	for (int lv = 0; lv <= maxlv; lv ++)
	{
		if (get_from(key, value, lv))
			return true;
	}

	return false;
}

template <typename ValueT>
inline bool SyncMyCache::get_and_cache(const std::string& key, ValueT& value, int cachelv /*= 0*/, int maxlevel /*= -1*/)
{
	const int maxlv = (maxlevel == -1 ? mycache::LV_GLOBAL : maxlevel);

	for (int lv = 0; lv <= maxlv; lv++)
	{
		if (get_from(key, value, lv))
		{
			if (lv != 0)
			{
				for (int i = 0; i <= cachelv; i++)
				{
					set_to(key, value, i);
				}
			}
			return true;
		}
	}

	return false;
}

template<typename ValueT>
inline bool SyncMyCache::set(const std::string& key, const ValueT& value, int level)
{
	return set_to(key, value, level);
}

inline bool SyncMyCache::del(const std::string& key, int maxlevel /*= -1*/)
{
	bool ret = true;
	const int maxlv = (maxlevel == -1 ? mycache::LV_GLOBAL : maxlevel);
	for (int lv = 0; lv <= maxlv; lv ++)
	{
		ret = del_from(key,lv);
	}
	return ret;
}


///////////////////////////////////////////////////////////////////////////////


template <typename ValueT>
inline bool AsyncMyCache::get_from(mycache::Get<ValueT>* cb, const std::string& key, int level)
{
	if (!cb) return false;

	cb->setKey(key);

	// in proc
	if (level <= 0)
	{
		ValueT value;
		bool ret = SyncMyCache::instance().get_from(key, value, level);
		
		if (ret)
			cb->result(&value);
		else
			cb->result(NULL);

		delete cb;
		return ret;
	}
	// from remote
	else
	{
		AsyncRedisClient* redis = client(level);
		if (redis)
		{
			return redis->call(cb);
		}
		else
		{
			cb->result(NULL);
			delete cb;
		}
	}

	return false;
}

template<typename ValueT>
inline bool AsyncMyCache::set_to(mycache::Set<ValueT>* cb, const std::string& key, const ValueT& value, int level)
{
	if (!cb) cb = new mycache::Set<ValueT>();
	if (!cb) return false;

	// to proc
	if (level <= 0)
	{
		bool ret = SyncMyCache::instance().set_to(key, value, level);
		cb->result(ret);
		delete cb;
		return ret;
	}
	else
	{	
		mycache::KVMap kvs;
		if (!value.saveToKVMap(kvs))
		{
			cb->result(false);
			delete cb;
			return false;
		}

		cb->setKVS(key, kvs);

		AsyncRedisClient* redis = client(level);
		if (redis)
		{
			return redis->call(cb);
		}
		else
		{
			cb->result(false);
			delete cb;
			return false;
		}
	}

	return false;
}

inline bool AsyncMyCache::del_from(mycache::Del* cb, const std::string& key, int level)
{
	if (!cb) cb = new mycache::Del;
	if (!cb) return false;

	cb->setKey(key);

	// from proc
	if (level <= 0)
	{
		bool ret = SyncMyCache::instance().del_from(key, level);
		cb->result(ret);
		delete cb;
		return ret;
	}
	else
	{
		AsyncRedisClient* redis = client(level);
		if (redis)
		{
			return redis->call(cb);
		}
		else
		{
			cb->result(false);
			delete cb;
			return false;
		}
	}

	return false;
}

template <typename ValueT>
inline bool AsyncMyCache::get(mycache::Get<ValueT>* cb, const std::string& key, int maxlevel /*= -1*/, int cacheit /*= -1*/)
{
	struct AutoGet : public mycache::Get<ValueT>
	{
	public:
		void result(const ValueT* value)
		{
			// hit
			if (value)
			{
				cb->result(value);

				if (level != 0 && cacheit != -1)
				{
					for (int i = 0; i <= cacheit; i++)
					{
						async_cache->set_to<ValueT>(NULL, this->getKey(), *value, i);
					}
				}

				//printf("hit : %s, %d \n", this->getKey().c_str(), level);
			}
			// miss
			else
			{
				//printf("miss : %s, %d \n", this->getKey().c_str(), level);

				// continue
				if (level < maxlevel)
				{
					AutoGet* getcb = new AutoGet();
					if (getcb)
					{
						getcb->setKey(this->getKey());
						getcb->level = level + 1;
						getcb->maxlevel = maxlevel;
						getcb->cacheit = cacheit;
						getcb->cb = cb;
						getcb->async_cache = async_cache;
						async_cache->get_from(getcb, this->getKey(), getcb->level);
					}
				}
				// finished
				else
				{
					cb->result(NULL);
					delete cb;
				}
			}
		}

	public:
		int level;
		int maxlevel;
		int cacheit;
		mycache::Get<ValueT>* cb;
		AsyncMyCache* async_cache;
	};

	AutoGet* getcb = new AutoGet();
	if (getcb)
	{
		getcb->setKey(key);
		getcb->level = 0;
		getcb->maxlevel = (maxlevel == -1 ? mycache::LV_GLOBAL : maxlevel);
		getcb->cacheit = cacheit;
		getcb->cb = cb;
		getcb->async_cache = this;
		return this->get_from(getcb, key, getcb->level);
	}

	return false;
}

template <typename ValueT>
inline bool AsyncMyCache::get_and_cache(mycache::Get<ValueT>* cb, const std::string& key, int cachelv /*= 0*/, int maxlevel /*= -1*/)
{
	return get<ValueT>(cb, key, maxlevel, cachelv);
}

template<typename ValueT>
inline bool AsyncMyCache::set(mycache::Set<ValueT>* cb, const std::string& key, const ValueT& value, int level)
{
	return set_to<ValueT>(cb, key, value, level);
}

inline bool AsyncMyCache::del(const std::string& key, int maxlevel /*= -1*/)
{
	bool ret = true;
	const int maxlv = (maxlevel == -1 ? mycache::LV_GLOBAL : maxlevel);
	for (int lv = 0; lv <= maxlv; lv ++)
	{
		ret = del_from(NULL, key, lv);
	}
	return ret;
}