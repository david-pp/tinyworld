#include "worker_pool.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN

WorkerPool::WorkerPool(size_t workernum) 
	: io_service_(new boost::asio::io_service),
		work_(new boost::asio::io_service::work(*io_service_)),
		workernum_(workernum)
{
}

bool WorkerPool::init()
{
	LOG4CXX_INFO(logger, "WorkerPool::init - " << workernum_);

	for (std::size_t i = 0; i < workernum_; ++i)
	{	
    	threads_.create_thread(boost::bind(&boost::asio::io_service::run, io_service_));
	}

	return true;
}

void WorkerPool::run()
{
	threads_.join_all();
}

void WorkerPool::fini()
{
    io_service_->stop();
    LOG4CXX_INFO(logger, "WorkerPool::fini - " << workernum_);
}

NAMESPACE_NETASIO_END