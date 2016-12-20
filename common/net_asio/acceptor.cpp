
#include "acceptor.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "mylogger.h"
//#include "protos/server.pb.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN


#if 0

static void onLogin(Cmd::Server::LoginRequest* msg, NetAsio::ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());

	Cmd::Server::LoginReply reply;
	reply.set_retcode(0);
	conn->send(reply);
}
#endif 


Acceptor::Acceptor(const std::string& address, const std::string& port, 
		IOServicePool& io_service_pool,
		WorkerPool& worker_pool,
		ProtobufMsgDispatcher& msgdispatcher) 
			: address_(address), port_(port),
			  io_service_pool_(io_service_pool),
	  		  workers_(worker_pool),
	  		  msgdispatcher_(msgdispatcher),
	  		  acceptor_(io_service_pool_.get_io_service()),
	  		  new_connection_()
{
	LOG4CXX_INFO(logger, "Acceptor::created - " << address_ << ":" << port_);
}

Acceptor::~Acceptor()
{
	LOG4CXX_INFO(logger, "Acceptor::destroyed - " << address_ << ":" << port_);
}

bool Acceptor::listen()
{
	LOG4CXX_INFO(logger, "Acceptor::listen - " << address_ << ":" << port_);

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
	boost::asio::ip::tcp::resolver::query query(address_, port_);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	//msgdispatcher_.bind<Cmd::Acceptor::LoginRequest, NetAsio::ConnectionPtr>(onLogin);

	start_accept();

	return true;
}

void Acceptor::close()
{
	acceptor_.close();
	this->stop_all();

	LOG4CXX_INFO(logger, "Acceptor::close - " << address_ << ":" << port_);
}

void Acceptor::start_accept()
{
	new_connection_.reset(new Session(
		io_service_pool_.get_io_service(),
		workers_.get_io_service(),
		boost::bind(&Acceptor::handle_message, this, _1, _2, _3)));

	new_connection_->set_id(0);
	new_connection_->set_name("session");
	new_connection_->set_read_error_callback(boost::bind(&Acceptor::handle_error, this, _1));
	new_connection_->set_write_error_callback(boost::bind(&Acceptor::handle_error, this, _1));

	acceptor_.async_accept(new_connection_->socket(),
		boost::bind(&Acceptor::handle_accept, this,
			boost::asio::placeholders::error));

	new_connection_->set_state(Connection::kState_Connecting);
}

void Acceptor::handle_accept(const boost::system::error_code& e)
{
	if (!e)
	{
		this->start(new_connection_);
		new_connection_->set_state(Connection::kState_Connected);	
	}
	else
	{
		LOG4CXX_ERROR(logger, "Acceptor::handle_accept - " << e.message());
	}

	start_accept();
}

void Acceptor::handle_message(ConnectionPtr conn, const char* msg, size_t msgsize)
{
	MessageHeader* mh = (MessageHeader*)msg;
	if (mh->msgsize() == msgsize)
	{
		ProtobufMsgHandler::MessagePtr protomsg = msgdispatcher_.makeMessage(
			mh->type, (void*)(msg + sizeof(MessageHeader)), mh->size);
	
		conn->strand().post(
			boost::bind(&ProtobufMsgDispatcher::dispatchMsg1<ConnectionPtr>, 
				msgdispatcher_, mh->type, protomsg, conn));

		//msgdispatcher_.dispatchMsg(mh->type, protomsg);
	}
}

void Acceptor::handle_error(ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "Acceptor::handle_error - " << address_ << ":" << port_);

	if (!conn->isClosed())
	{
		conn->set_state(Connection::kState_Closed);
		this->stop(conn);
	}
}

NAMESPACE_NETASIO_END