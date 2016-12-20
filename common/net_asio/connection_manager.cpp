#include "connection_manager.h"
#include <algorithm>
#include <boost/bind.hpp>

NAMESPACE_NETASIO_BEGIN

void ConnectionManager::start(ConnectionPtr c)
{
	boost::mutex::scoped_lock slock(connections_lock_);
	connections_.insert(std::make_pair(c->id(), c));
	c->start();
}

void ConnectionManager::stop(ConnectionPtr c)
{
	if (c)
		stop(c->id());
}

void ConnectionManager::stop(uint32 id)
{
	boost::mutex::scoped_lock slock(connections_lock_);

	ConnectionMap::iterator it = connections_.find(id);
	if (it != connections_.end())
	{
		it->second->stop();
		connections_.erase(it);
	}
}

void ConnectionManager::stop_all()
{
	boost::mutex::scoped_lock slock(connections_lock_);

	for (ConnectionMap::iterator it = connections_.begin(); it != connections_.end(); ++ it)
		it->second->stop();

	connections_.clear();
}

NAMESPACE_NETASIO_END