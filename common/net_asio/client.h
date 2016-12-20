#ifndef _NET_ASIO_CLIENT_H
#define _NET_ASIO_CLIENT_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include "connection.h"
#include "message_dispatcher.h"

NAMESPACE_NETASIO_BEGIN

class Client : public Connection
{
public:
	Client(const std::string& server, const std::string& port,
		boost::asio::io_service& io_service,
		boost::asio::io_service& worker_service,
		ProtobufMsgDispatcher& msgdispatcher, 
		uint32 retry_interval = 0);

	virtual void start();

	virtual void stop();

protected:
	/// Initiate an asynchronous connect operation.
	void start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

	/// Handele resolved result
	void handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

	/// Handle connect
	void handle_connect(const boost::system::error_code& err);

	/// Handle received message
	void handle_message(ConnectionPtr, const char*, size_t);

	/// 检查断线并重连定时器回调
	void handle_retry(const boost::system::error_code& e);

	virtual void onReadError(const boost::system::error_code& e);
  	virtual void onWriteError(const boost::system::error_code& e);

private:
	/// 服务器地址和端口
	std::string address_;
	std::string port_;

	/// 断线重连时间间隔
	uint32 retry_interval_;
	boost::asio::deadline_timer retry_timer_;

	/// 地址解析器
	boost::asio::ip::tcp::resolver resolver_;

	/// 消息分发器
	ProtobufMsgDispatcher& msgdispatcher_;
};

typedef boost::shared_ptr<Client> ClientPtr;


NAMESPACE_NETASIO_END

#endif // _NET_ASIO_CLIENT_H