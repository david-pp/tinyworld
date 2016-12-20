#ifndef _NET_ASIO_ACCPTOR_H
#define _NET_ASIO_ACCPTOR_H

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "session.h"
#include "connection_manager.h"
#include "io_service_pool.h"
#include "worker_pool.h"
#include "message_dispatcher.h"

NAMESPACE_NETASIO_BEGIN

/////////////////////////////////////////////////////////////////////
//
// TCP服务端
//
/////////////////////////////////////////////////////////////////////
class Acceptor : public ConnectionManager
{
public:
	Acceptor(const std::string& address, const std::string& port, 
		IOServicePool& io_service_pool,
		WorkerPool& worker_pool,
		ProtobufMsgDispatcher& msgdispatcher);

	~Acceptor();

	/// 绑定端口、开始监听
	bool listen();

	/// 关闭所有连接
	void close();

protected:
	/// Initiate an asynchronous accept operation.
	void start_accept();

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& e);

	/// Handle received message
	void handle_message(ConnectionPtr, const char*, size_t);

	/// Handle R/W error
	void handle_error(ConnectionPtr);

private:
	/// address
	std::string address_;
	/// port
	std::string port_;

	/// The pool of io_service objects used to perform asynchronous operations.
	IOServicePool& io_service_pool_;

	/// Message Handler threads
	WorkerPool& workers_;

	/// The handler for all incoming requests.
	ProtobufMsgDispatcher& msgdispatcher_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// The next connection to be accepted.
	ConnectionPtr new_connection_;
};

// typedef boost::shared_ptr<Acceptor> AcceptorPtr;

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_ACCPTOR_H
