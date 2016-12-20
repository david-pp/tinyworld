#ifndef __COMMON_SYNC_H_
#define __COMMON_SYNC_H_

namespace sync {

	// connect
	bool connect(const std::string& address, int hashcode = 0);

	// bind
	bool bind(const std::string& address);

	// push
	bool push(const std::string& key, void* value);
	bool pushToGroup(const std::string& key, void* value);
	bool pushToAll(const std::string& key, void* value);

	// pull
	void* pull(const std::string& key);
	void* pullFromCache(const std::string& key);
	void* pullFromCloud(const std::string& key);
}

#endif // __COMMON_SYNC_H_