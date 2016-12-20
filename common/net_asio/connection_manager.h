#ifndef _NET_ASIO_CONNECTION_MANAGER_H
#define _NET_ASIO_CONNECTION_MANAGER_H

#include <set>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include "connection.h"
#include "net.h"

NAMESPACE_NETASIO_BEGIN

////////////////////////////////////////////////////////
//
// TCP连接管理器
//
////////////////////////////////////////////////////////
class ConnectionManager : private boost::noncopyable
{
public:
	/// Add the specified connection to the manager and start it.
	void start(ConnectionPtr c);

	/// Stop the specified connection.
	void stop(ConnectionPtr c);
	void stop(uint32 id);

	/// Stop all connections.
	void stop_all();

	template <typename ProtoT>
	void sendTo(uint32 id, const ProtoT& proto)
	{
		boost::mutex::scoped_lock slock(connections_lock_);

		ConnectionMap::iterator it = connections_.find(id);
		if (it != connections_.end())
		{
			it->second->send(proto);
		}
	}

	template <typename ProtoT>
	void broadcastTo(uint32 type, const ProtoT& proto)
	{
		boost::mutex::scoped_lock slock(connections_lock_);

		for (ConnectionMap::iterator it = connections_.begin(); 
			it != connections_.end(); ++ it)
			if (it->second->type() == type)
				it->second->send(proto);
	}

	template <typename ProtoT>
	void broadcastToAll(const ProtoT& proto)
	{
		boost::mutex::scoped_lock slock(connections_lock_);

		for (ConnectionMap::iterator it = connections_.begin(); 
			it != connections_.end(); ++ it)
			it->second->send(proto);
	}

	size_t size() { return connections_.size(); }

private:
	typedef std::map<uint32, ConnectionPtr> ConnectionMap;
	/// The managed connections.
	ConnectionMap connections_;

	boost::mutex connections_lock_;
};

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_CONNECTION_MANAGER_H
