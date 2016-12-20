#ifndef _NET_ASIO_IOServicePool_H
#define _NET_ASIO_IOServicePool_H

#include "net.h"
#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

NAMESPACE_NETASIO_BEGIN

/// A pool of io_service objects.
class IOServicePool : private boost::noncopyable
{
public:
	/// Construct the io_service pool.
	IOServicePool(std::size_t pool_size);

	bool init();

	/// Run all io_service objects in the pool.
	void run();

	/// Stop all io_service objects in the pool.
	void fini();

	/// Get an io_service to use.
	boost::asio::io_service& get_io_service();

private:
	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

	/// The pool of io_services.
	std::vector<io_service_ptr> io_services_;

	/// The work that keeps the io_services running.
	std::vector<work_ptr> work_;

	std::size_t pool_size_;

	/// The next io_service to use for a connection.
	std::size_t next_io_service_;

	boost::thread_group threads_;
};


NAMESPACE_NETASIO_END

#endif // _NET_ASIO_IOServicePool_H
