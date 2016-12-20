#include "client.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "mylogger.h"
//#include "protos/server.pb.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN

#if 0
static void onLoginReply(Cmd::Server::LoginReply* msg, NetAsio::ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());
}

#endif

Client::Client(const std::string& server, const std::string& port,
		boost::asio::io_service& io_service,
		boost::asio::io_service& worker_service,
		ProtobufMsgDispatcher& msgdispatcher, 
		uint32 retry_interval) :
			Connection(io_service, worker_service, boost::bind(&Client::handle_message, this, _1, _2, _3)),
			address_(server),
			port_(port),
			retry_interval_(retry_interval), 
			retry_timer_(io_service, boost::posix_time::millisec(retry_interval)),
			resolver_(io_service),
			msgdispatcher_(msgdispatcher)

{
	if (retry_interval_ > 0)
	{
		retry_timer_.async_wait(
			boost::bind(&Client::handle_retry, this,
				boost::asio::placeholders::error));
	}

	set_name("client");

	//msgdispatcher_.bind<Cmd::Server::LoginReply, NetAsio::ConnectionPtr>(onLoginReply);
}

void Client::start()
{
	// Start an asynchronous resolve to translate the server and service names
	// into a list of endpoints.
	boost::asio::ip::tcp::resolver::query query(address_, port_);
	resolver_.async_resolve(query,
		boost::bind(&Client::handle_resolve, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));

	set_state(kState_Resolving);
}

void Client::stop()
{
	set_state(kState_Closed);
}

void Client::handle_resolve(const boost::system::error_code& err, 
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{

	if (err)
	{
		LOG4CXX_ERROR(logger, "Client::handle_resolve:" << err.message());
		set_state(kState_Closed);
		return;
	}

	set_state(kState_Resolved);

	boost::asio::ip::tcp::endpoint ep = *endpoint_iterator;
	LOG4CXX_INFO(logger, "Client::handle_resolve: " << ep);

	start_connect(endpoint_iterator);
}


void Client::start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	// Attempt a connection to each endpoint in the list until we
	// successfully establish a connection.
	boost::asio::async_connect(socket_, endpoint_iterator,
		boost::bind(&Client::handle_connect, this,
			boost::asio::placeholders::error));

	set_state(kState_Connecting);
}

void Client::handle_connect(const boost::system::error_code& err)
{
	if (err)
	{
		LOG4CXX_ERROR(logger, "Client::handle_connect:" << err.message());
		set_state(kState_Closed);
		return;
	}

	Connection::start();

	set_state(kState_Connected);

/*
	// send to server
	Cmd::Server::LoginRequest login;
	login.set_id(id_);
	login.set_type(type_);
	this->send(login);
	*/
}

void Client::handle_message(ConnectionPtr conn, const char* msg, size_t msgsize)
{
	LOG4CXX_INFO(logger, "Client::handle_message");

	MessageHeader* mh = (MessageHeader*)msg;
	if (mh->msgsize() == msgsize)
	{
		ProtobufMsgHandler::MessagePtr protomsg = msgdispatcher_.makeMessage(
			mh->type, (void*)(msg + sizeof(MessageHeader)), mh->size);
		if (protomsg)
		{
			strand_.post(
				boost::bind(&ProtobufMsgDispatcher::dispatchMsg1<ConnectionPtr>, 
					msgdispatcher_, mh->type, protomsg, conn));

			//msgdispatcher_.dispatchMsg1(mh->type, protomsg, conn);
		}
		else
		{
			LOG4CXX_ERROR(logger, "Client::handle_message:Invalid msg type:" << mh->type);
		}
	}
}

void Client::handle_retry(const boost::system::error_code& e)
{
	if (isClosed())
	{
		LOG4CXX_INFO(logger, "Client::handle_retry : " << retry_interval_);
		start();
	}

	retry_timer_.expires_at(retry_timer_.expires_at() + boost::posix_time::millisec(retry_interval_));
	retry_timer_.async_wait(
		boost::bind(&Client::handle_retry, this,
			boost::asio::placeholders::error));
}

void Client::onReadError(const boost::system::error_code& e)
{
	set_state(kState_Closed);
}
 
void Client::onWriteError(const boost::system::error_code& e)
{
	set_state(kState_Closed);
}

NAMESPACE_NETASIO_END