#include "session_manager.h"
#include <algorithm>
#include <boost/bind.hpp>

NAMESPACE_NETASIO_BEGIN

void SessionManager::start(ConnectionPtr c)
{
	boost::mutex::scoped_lock slock(lock_);
	connections_.insert(c);
	c->start();
}

void SessionManager::stop(ConnectionPtr c)
{
	boost::mutex::scoped_lock slock(lock_);
	connections_.erase(c);
	c->stop();
}

void SessionManager::stop_all()
{
	boost::mutex::scoped_lock slock(lock_);
	std::for_each(connections_.begin(), connections_.end(),
		boost::bind(&Connection::stop, _1));
	connections_.clear();
}

NAMESPACE_NETASIO_END