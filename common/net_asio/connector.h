#ifndef _NET_ASIO_CONNECTOR_H
#define _NET_ASIO_CONNECTOR_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "net.h"
#include "client.h"
#include "io_service_pool.h"
#include "worker_pool.h"
#include "message_dispatcher.h"
#include "connection_manager.h"

NAMESPACE_NETASIO_BEGIN

//////////////////////////////////////////////////////////
//
// 连接端子：可以发起多条客户端连接
//
//////////////////////////////////////////////////////////
class Connector : public ConnectionManager
{
public:
	Connector(IOServicePool& io_service_pool,
		WorkerPool& worker_pool,
		ProtobufMsgDispatcher& msgdispatcher);

	//
	// 发起一条客户端连接:
	//  id   - 唯一标识(0则自动生成临时ID)
	//  type - 连接的服务器类型
	//  address - 服务器地址
	//  port    - 服务器端口
	//  retryms - 自动重连定时器间隔/ms
	//
	bool connect(uint32 id, uint32 type, const std::string& address, const std::string& port, uint32 retryms=0);

	//
	// 关闭并释放所有客户端连接
	//
	void close();

private:
	/// The pool of io_service objects used to perform asynchronous operations.
	IOServicePool& io_service_pool_;

	/// Woker threads
	WorkerPool& worker_pool_;

	/// The handler for all incoming requests.
	ProtobufMsgDispatcher& msgdispatcher_;
};


NAMESPACE_NETASIO_END

#endif // _NET_ASIO_CONNECTOR_H