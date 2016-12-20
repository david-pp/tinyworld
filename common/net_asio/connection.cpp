#include "connection.h"
#include <sstream>
#include <boost/thread.hpp>
#include "mylogger.h"

NAMESPACE_NETASIO_BEGIN

static LoggerPtr logger(Logger::getLogger("tinyserver"));

namespace {

	template <typename T>
	struct IDAllocator
	{
	public:
		IDAllocator(T start) : id(start) {}

		T acquire()
		{
			boost::mutex::scoped_lock sl(lock);
			++id;
			return id;
		}

	public:
		T id;
		boost::mutex lock;
	};

	IDAllocator<uint32> id_allocator(10000);
}

Connection::Connection(boost::asio::io_service& io_service,
		boost::asio::io_service& strand_service,
		const MessageHandler& message_handler) 
		: socket_(io_service), 
		  strand_(strand_service),
		  message_handler_(message_handler)
{
	id_ = 0;
	type_ = 0;
	state_ = 0;

	LOG4CXX_INFO(logger, debugString() << " - created...");
}

Connection::~Connection()
{
	LOG4CXX_INFO(logger, debugString() << " - destroyed...");
}

std::string Connection::debugString()
{
	char debug[256] = "";
	snprintf(debug, sizeof(debug), "[%s:%u]", name_.c_str(), id_);
	return debug;
}

const char* Connection::stateDescription(uint16 state)
{
	switch (state)
	{
		case kState_Closed:
			return "closed";
		case kState_Resolving:
			return "resolving";
		case kState_Resolved:
			return "resolved";
		case kState_Connecting:
			return "connecting";
		case kState_Connected:
			return "connected";
		case kState_Verifying:
			return "verifying";
		case kState_Verified:
			return "verified";
		case kState_Runing:
			return "running";
	}
	return "invalid";
}

void Connection::set_state(ConnectionStateEnum state)
{
	uint16 oldstate = state_;

	state_ = state;

	LOG4CXX_INFO(logger, debugString() << " - state change : " 
		<< stateDescription(oldstate) 
		<< "->"
		<< stateDescription(state_));

	if (on_state_change_)
		on_state_change_(shared_from_this(), oldstate, state_);
}

void Connection::set_id(const uint32 id)
{
	if (id)
		id_ = id;
	else
		id_ = id_allocator.acquire();

	LOG4CXX_INFO(logger, debugString() << " - set_id...");
}

void Connection::start()
{
	LOG4CXX_INFO(logger, debugString() << " - start...");
	asyncRead();
}

void Connection::stop()
{
	LOG4CXX_INFO(logger, debugString() << " - stop...");
	socket_.close();

	// Initiate graceful connection closure.
	//boost::system::error_code ignored_ec;
	//socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void Connection::asyncRead()
{
	boost::asio::async_read(socket_, 
		readbuf_,
		boost::asio::transfer_at_least(1),
		boost::bind(&Connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error));
}
 
void Connection::asyncWrite(SharedBuffer buff)
{
	boost::asio::async_write(socket_,
		boost::asio::buffer(buff.asio_buff()),
		boost::bind(&Connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error, buff));
}

bool Connection::readMessages()
{
	while (readbuf_.size() >= sizeof(MessageHeader))
	{
		const MessageHeader* mh = boost::asio::buffer_cast<const MessageHeader*>(
			readbuf_.data());
	    if (mh->size > 8192)
	    {
	    	return false;
	    }

	    const size_t msgsize = sizeof(MessageHeader) + mh->size;
	    if (readbuf_.size() >= msgsize)
	    {
	    	#if 0
	    	LOG4CXX_INFO(logger, "proto : size=" << mh->size 
	    		<< ", type1=" << (int)mh->type_first
	    		<< ", type2=" << (int)mh->type_second);
	    	#endif

	    	if (message_handler_)
	    		message_handler_(shared_from_this(), 
	    			boost::asio::buffer_cast<const char*>(readbuf_.data()),
	    			msgsize);

	    	readbuf_.consume(msgsize);
	    }
	}

  return true;
}

void Connection::handle_read(const boost::system::error_code& e)
{
	if (!e)
	{
		LOG4CXX_INFO(logger, debugString() << " - handle_read:" << readbuf_.size());

		if (readMessages())
		{
			asyncRead();
		}
		else
		{
			this->onReadError(e);
		}
	}
	else
	{
		LOG4CXX_ERROR(logger, debugString() << " - handle_read:" << e.message());

		this->onReadError(e);
	}
}

void Connection::handle_write(const boost::system::error_code& e, SharedBuffer buff)
{
	if (!e)
	{
		LOG4CXX_INFO(logger, debugString() << " - handle_write:" << buff.size);
	}
	else
	{
		LOG4CXX_ERROR(logger, debugString() << " - handle_write:" << e.message());

		this->onWriteError(e);
	}
}

void Connection::onReadError(const boost::system::error_code& e)
{
	if (on_read_err_)
	{
		on_read_err_(shared_from_this());
	}
}

void Connection::onWriteError(const boost::system::error_code& e)
{
	if (on_write_err_)
	{
		on_write_err_(shared_from_this());
	}
}

NAMESPACE_NETASIO_END