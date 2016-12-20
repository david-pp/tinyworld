#include "connector.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("tinyserver"));

NAMESPACE_NETASIO_BEGIN

Connector::Connector(IOServicePool& io_service_pool,
		WorkerPool& worker_pool,
		ProtobufMsgDispatcher& msgdispatcher) 
			: io_service_pool_(io_service_pool),
			  worker_pool_(worker_pool),
			  msgdispatcher_(msgdispatcher)
{
}

bool Connector::connect(uint32 id, uint32 type, const std::string& address, const std::string& port, uint32 retryms)
{
	NetAsio::ConnectionPtr conn(new NetAsio::Client(address, port, 
		io_service_pool_.get_io_service(),
		worker_pool_.get_io_service(), msgdispatcher_, retryms));
	if (conn)
	{
		conn->set_id(id);
		conn->set_type(type);
		
		this->start(conn);
		return true;
	}

	return false;
}

void Connector::close()
{
	this->stop_all();
}

NAMESPACE_NETASIO_END