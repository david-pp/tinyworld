#ifndef _NET_ASIO_SESSION_MANAGER_HPP
#define _NET_ASIO_SESSION_MANAGER_HPP

#include <set>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include "session.h"
#include "net.h"

NAMESPACE_NETASIO_BEGIN

class SessionManager
	: private boost::noncopyable
{
public:
	/// Add the specified connection to the manager and start it.
	void start(ConnectionPtr c);

	/// Stop the specified connection.
	void stop(ConnectionPtr c);

	/// Stop all connections.
	void stop_all();

private:
	/// The managed connections.
	std::set<ConnectionPtr> connections_;

	boost::mutex lock_;
};

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_SESSION_MANAGER_HPP
