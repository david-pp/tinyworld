#ifndef _NET_ASIO_SESSION_H
#define _NET_ASIO_SESSION_H

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "connection.h"

NAMESPACE_NETASIO_BEGIN

class SessionManager;

/// Represents a single connection from a client.
class Session : public Connection 
{
public:
	/// Construct a connection with the given io_service.
	explicit Session(boost::asio::io_service& io_service,
		boost::asio::io_service& worker_service,
		MessageHandler handler);

	virtual ~Session();

	#if 0

	virtual void close();

	virtual void onReadError(const boost::system::error_code& e);
  	virtual void onWriteError(const boost::system::error_code& e);
  	#endif

private:
	/// The manager for this connection.
	//SessionManager& session_manager_;
};

typedef boost::shared_ptr<Session> SessionPtr;

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_SESSION_H
