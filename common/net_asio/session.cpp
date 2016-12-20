
#include "session.h"
#include <vector>
#include <boost/bind.hpp>

#include "session_manager.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN

Session::Session(boost::asio::io_service& io_service,
    boost::asio::io_service& worker_service,
    MessageHandler handler)
  : Connection(io_service, worker_service, handler)
{
}

Session::~Session()
{
}

#if 0

void Session::close()
{
	session_manager_.stop(shared_from_this());
}

void Session::onReadError(const boost::system::error_code& e)
{
	session_manager_.stop(shared_from_this());
}

void Session::onWriteError(const boost::system::error_code& e)
{
	session_manager_.stop(shared_from_this());
}

#endif

NAMESPACE_NETASIO_END