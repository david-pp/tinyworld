#ifndef _NET_ASIO_SERVICE_WORKER_H
#define _NET_ASIO_SERVICE_WORKER_H

#include "net.h"
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

NAMESPACE_NETASIO_BEGIN

///////////////////////////////////////////////////
//
// 消息处理线程池
//
///////////////////////////////////////////////////
class WorkerPool : private boost::noncopyable
{
public:
	WorkerPool(size_t workernum = 1);

	bool init();

	void run();

  	void fini();

  	boost::asio::io_service& get_io_service() { return *io_service_; }

private:
	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
  	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

	io_service_ptr io_service_;
	work_ptr work_;
	size_t workernum_;

	boost::thread_group threads_;
};

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_SERVICE_WORKER_H
