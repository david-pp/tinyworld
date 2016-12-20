#include "io_service_pool.h"
#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN

IOServicePool::IOServicePool(std::size_t pool_size)
	: pool_size_(pool_size),
	  next_io_service_(0)
{
}

bool IOServicePool::init()
{
	LOG4CXX_INFO(logger, "IOServicePool::init - " << pool_size_);

	if (pool_size_ == 0)
	{
		LOG4CXX_ERROR(logger, "IOServicePool size is 0");
		return false;
	}

	// Give all the io_services work to do so that their run() functions will not
	// exit until they are explicitly stopped.
	for (std::size_t i = 0; i < pool_size_; ++i)
	{
		io_service_ptr io_service(new boost::asio::io_service);
		work_ptr work(new boost::asio::io_service::work(*io_service));
		io_services_.push_back(io_service);
		work_.push_back(work);
	}

	for (std::size_t i = 0; i < io_services_.size(); ++i)
	{
		threads_.create_thread(boost::bind(&boost::asio::io_service::run, io_services_[i]));
	}

	return true;
}

void IOServicePool::run()
{
	// Wait for all threads in the pool to exit.
	threads_.join_all();
}

void IOServicePool::fini()
{
	LOG4CXX_INFO(logger, "IOServicePool::fini - " << pool_size_);

	// Explicitly stop all io_services.
	for (std::size_t i = 0; i < io_services_.size(); ++i)
		io_services_[i]->stop();

	io_services_.clear();
}

boost::asio::io_service& IOServicePool::get_io_service()
{
	// Use a round-robin scheme to choose the next io_service to use.
	boost::asio::io_service& io_service = *io_services_[next_io_service_];
	++next_io_service_;
	if (next_io_service_ == io_services_.size())
		next_io_service_ = 0;
	return io_service;
}

NAMESPACE_NETASIO_END
