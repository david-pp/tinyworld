#include "mycache.h"
#include "zmq.hpp"
#include <string>
#include <iostream>
#include <cstdarg>

#include "mylogger.h"
#include "mycache.pb.h"

static LoggerPtr logger(Logger::getLogger("mycache"));

bool MyCache::packBinMsg(zmq::message_t& message, uint64_t id, uint8_t cmd, const std::string& msg)
{
	message.rebuild(sizeof(cmd) + sizeof(id) + msg.size());

	uint8_t* msgdata = (uint8_t*)message.data();

	// cmdtype
	memcpy(msgdata, &cmd, sizeof(cmd));
	msgdata += sizeof(cmd);

	// id
	memcpy(msgdata, &id, sizeof(id));
	msgdata += sizeof(id);

	// msgbody
	memcpy(msgdata, msg.data(), msg.size());
	msgdata += msg.size();

	return true;
}


bool MyCache::parseBinMsg(zmq::message_t& message, uint64_t& id, uint8_t& cmd, std::string& msgbody)
{
	if (message.size() < (sizeof(cmd) + sizeof(id)))
		return false;

	size_t bodylen = message.size() - (sizeof(cmd) + sizeof(id));

	uint8_t* msgdata = (uint8_t*)message.data();

	// cmdtype
	memcpy(&cmd, msgdata, sizeof(cmd));
	msgdata += sizeof(cmd);

	// id
	memcpy(&id, msgdata, sizeof(id));
	msgdata += sizeof(id);

	// msgbody
	if (bodylen > 0)
	{
		msgbody.assign((char*)msgdata, bodylen);
	}

	return true;
}

/////////////////////////////////////////////

std::string formatKey(const char* format, ...)
{
	char key[256] = "";

	va_list ap;
	va_start(ap, format);
	vsnprintf(key, sizeof(key), format, ap);
	va_end(ap);

	return key;
}


MyCacheClient::MyCacheClient(const std::string& id, zmq::context_t* context)
{
	address_ = "tcp://localhost:5555";
	socket_ = NULL;
	context_ = (context ? context : new zmq::context_t(1));
	id_ = id;
}

MyCacheClient::~MyCacheClient()
{
	close();
	if (context_)
		delete context_;
}

bool MyCacheClient::connect(const std::string& serveraddr)
{
	close();

	address_ = serveraddr;

	try 
	{
    	socket_ = new zmq::socket_t (*context_, ZMQ_REQ);
    	if (socket_)
    	{
    		if (!id_.empty())
    		{
	    		socket_->setsockopt (ZMQ_IDENTITY, id_.data(), id_.size());
	    	}

	    	//  Configure socket to not wait at close time
	    	int linger = 0;
	    	socket_->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));

    		socket_->connect (address_.c_str());

    		LOG4CXX_INFO(logger, "connect mycached success: " << address_ << ":" << pthread_self()); 	
	    	
	    	return true;
    	}
    }
    catch (std::exception& err)
    {
    	close();
    	LOG4CXX_ERROR(logger, "connect failed : " << err.what());
    }

    return false;
}

void MyCacheClient::close()
{
	if (socket_)
	{
		delete socket_;
		socket_ = NULL;
	}
}

#define REQUEST_RETRIES     3   //  Before we abandon

bool MyCacheClient::request(std::string& reply, unsigned char cmdtype, const std::string& msg, int timeoutms)
{
    int retries_left = REQUEST_RETRIES;

    while (retries_left) 
    {
    	try 
    	{
    		if (!socket_)
    			connect(address_);

    		if (!socket_)
    		{
    			LOG4CXX_ERROR(logger, "request failed: socket is closed");
    			return false;
    		}

    		MyCache::sendBinMsg(socket_, 0, cmdtype, msg);

	        bool expect_reply = true;
	        while (expect_reply)
	        {
	            //  Poll socket for a reply, with timeout
	            zmq::pollitem_t items[] = { { *socket_, 0, ZMQ_POLLIN, 0 } };
	            zmq::poll (&items[0], 1, timeoutms);

	            //  If we got a reply, process it
	            if (items[0].revents & ZMQ_POLLIN) 
	            {
	           		//  We got a reply from the server, must match sequence
	                zmq::message_t reply_msg;
	                socket_->recv(&reply_msg);

	            	uint64_t id = 0;
		    		uint8_t cmd = 0;
			    	if (MyCache::parseBinMsg(reply_msg, id, cmd, reply))
			    	{
			    		return true;
			    	}
			    	else
			    	{
			    		LOG4CXX_WARN(logger, "parse msg failed: id=" << id  
			    			<< " msgbody=" << reply.size());
			    	}
			    	return false;
	            }
	            else if (--retries_left == 0)
	            {
	            	LOG4CXX_WARN(logger, "request failed: server seems to be offline, abandoning");
	                expect_reply = false;
	                break;
	            }
	            else 
	            {
	                LOG4CXX_WARN(logger, "request failed: no response from server, retrying...");
	                
	                //  Old socket will be confused; close it and open a new one
	               	connect(address_);

	               	MyCache::sendBinMsg(socket_, 0, cmdtype, msg);
	            }
	        }
	    }
	    catch(const std::exception& err)
	    {
	    	LOG4CXX_ERROR(logger, "request failed: " << cmdtype << ":" << err.what());
	    	close();
	    	break;
	    }
    }

    return false;
}

bool MyCacheClient::get(const std::string& key, std::string& value, int timeoutms)
{
	return request(value, kMyCacheCmd_Get, key, timeoutms);
}

bool MyCacheClient::set(const std::string& key, const std::string& value, int timeoutms)
{
	Cmd::Set setmsg;
	setmsg.set_key(key);
	setmsg.set_value(value);

	std::string msg;
	setmsg.SerializeToString(&msg);

	std::string ret;
	return request(ret, kMyCacheCmd_Set, msg, timeoutms) && ret == "OK";
}

bool MyCacheClient::del(const std::string& key, int timeoutms)
{
	std::string ret;
	return request(ret, kMyCacheCmd_Del, key, timeoutms)  && ret == "OK";
}

/////////////////////////////

AsyncMyCacheClient::AsyncMyCacheClient(const std::string& id, zmq::context_t* context)
{
	address_ = "tcp://localhost:5555";
	socket_ = NULL;
	context_ = (context ? context : new zmq::context_t(1));
	id_ = id;
}

AsyncMyCacheClient::~AsyncMyCacheClient()
{
	close();
	if (context_)
		delete context_;
}

bool AsyncMyCacheClient::connect(const std::string& serveraddr)
{
	close();

	address_ = serveraddr;

	try 
	{
    	socket_ = new zmq::socket_t (*context_, ZMQ_DEALER);
    	if (socket_)
    	{
    		std::string id;
    		if (id_.empty())
    		{
    			std::stringstream ss;
    			ss << "_";
			    ss << std::hex << std::uppercase
			       << std::setw(4) << std::setfill('0') << within (0x10000) << "-"
			       << std::setw(4) << std::setfill('0') << within (0x10000);
			    id = ss.str();
    		}
    		else
    		{
    			id = "_" + id_;
    		}

	    	socket_->setsockopt (ZMQ_IDENTITY, id.data(), id.size());
    		socket_->connect (address_.c_str());

    		LOG4CXX_INFO(logger, "async connect mycached success: " 
    			<< id << ":"
    			<< address_ 
    			<< ":" << pthread_self()); 	
	    	
	    	return true;
    	}
    }
    catch (std::exception& err)
    {
    	close();
    	LOG4CXX_ERROR(logger, "async connect failed : " << address_ << " : " << err.what());
    }

    return false;
}

void AsyncMyCacheClient::close()
{
	if (socket_)
	{
		delete socket_;
		socket_ = NULL;
	}
}

void AsyncMyCacheClient::run()
{
	while (true)
	{
		try 
		{
			loop();
        }
		catch (std::exception& err)
		{
			std::cout << "!!!!" << err.what() << std::endl;
		}
	}
}

bool AsyncMyCacheClient::loop()
{
	try 
	{
		zmq::pollitem_t items[] = { { *socket_, 0, ZMQ_POLLIN, 0 } };

		int rc = zmq_poll (&items[0], 1, 100);
		if (-1 != rc)
		{
		    //  If we got a reply, process it
		    if (items[0].revents & ZMQ_POLLIN) 
		    {
			    zmq::message_t reply;
			    socket_->recv (&reply);

			    if (reply.size() > 0)
			    {
			    	//LOG4CXX_INFO(logger, "fuck:" << (char*)reply.data());

			    	uint64_t id = 0;
			    	uint8_t cmd = 0;
			    	std::string msgbody;
			    	if (MyCache::parseBinMsg(reply, id, cmd, msgbody))
			    	{
			    		this->onRecv(id, cmd, msgbody);			    	
			    	}
			    	else
			    	{
			    		LOG4CXX_WARN(logger, "parse msg failed: id=" << id  
			    			<< " msgbody=" << msgbody.size());
			    	}
			    }
			}
		}

		this->onIdle();

		return true;
	}
	catch (std::exception& err)
	{
		LOG4CXX_ERROR(logger, "AsyncMyCacheClient::loop - " << err.what());
		return false;
	}
}

void AsyncMyCacheClient::onIdle()
{
	scheduler_.run();
}

void AsyncMyCacheClient::onRecv(uint64_t id, uint8_t cmd, std::string& msgbody)
{
	scheduler_.triggerCall(id, &msgbody);
}

bool AsyncMyCacheClient::call(MyCache::RPC* rpc, const ContextPtr& context, int timeoutms)
{
	if (!rpc) return false;

	rpc->client = this;
	scheduler_.call(rpc, context, timeoutms);
	return true;
}

bool AsyncMyCacheClient::callBySerials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (cb)
	{
		for (size_t i = 0; i < rpcs.size(); i++)
		{
			MyCache::RPC* child = (MyCache::RPC*)rpcs[i];
			child->client = this;
		}

		return scheduler_.callBySerials(cb, rpcs, context, timeoutms);
	}
	return false;
}

bool AsyncMyCacheClient::callByParallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (cb)
	{
		for (size_t i = 0; i < rpcs.size(); i++)
		{
			MyCache::RPC* child = (MyCache::RPC*)rpcs[i];
			child->client = this;
		}
		return scheduler_.callByParallel(cb, rpcs, context, timeoutms);
	}
	return false;
}

bool AsyncMyCacheClient::callBySerials(SerialsCallBack* cb, const ContextPtr& context, MyCache::RPC* rpc1, ...)
{
	AsyncRPC::PtrArray rpcs;

	va_list ap;
	va_start(ap, rpc1);
	rpcs = AsyncRPC::S_v(rpc1, ap);
	va_end(ap);

	return callBySerials(cb, rpcs, context, -1);
}

bool AsyncMyCacheClient::callByParallel(ParallelCallBack* cb, const ContextPtr& context, MyCache::RPC* rpc1, ...)
{
	AsyncRPC::PtrArray rpcs;

	va_list ap;
	va_start(ap, rpc1);
	rpcs = AsyncRPC::S_v(rpc1, ap);
	va_end(ap);

	return callByParallel(cb, rpcs, context, -1);
}


/////////////////////////////

bool MyCache::init(const std::string& address, unsigned int maxconns)
{
	MyCachePool::instance()->setServerAddress(address);
	MyCachePool::instance()->setMaxConn(maxconns);
	MyCachePool::instance()->createAll();
	return true;
}

bool MyCache::get(const std::string& key, std::string& value, int timeoutms)
{
	ScopedMyCache mycache;
	if (mycache)
	{
		return mycache->get(key, value, timeoutms);
	}
	return false;
}

bool MyCache::set(const std::string& key, const std::string& value, int timeoutms)
{
	ScopedMyCache mycache;
	if (mycache)
	{
		return mycache->set(key, value, timeoutms);
	}
	return false;
}

bool MyCache::del(const std::string& key, int timeoutms)
{
	ScopedMyCache mycache;
	if (mycache)
	{
		return mycache->del(key, timeoutms);
	}
	return false;
}

static AsyncMyCacheClient* async = NULL;

bool MyCache::async_init(const std::string& address, const std::string& id)
{
	async = new AsyncMyCacheClient(id);
	if (async)
	{
		return async->connect(address);
	}

	return false;
}

bool MyCache::async_call(RPC* rpc, const ContextPtr& context, int timeoutms)
{
	if (async)
	{
		return async->call(rpc, context, timeoutms);
	}

	return false;
}

bool MyCache::async_serials(SerialsCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (async)
	{
		return async->callBySerials(cb, rpcs, context, timeoutms);
	}

	return false;
}

bool MyCache::async_parallel(ParallelCallBack* cb, const AsyncRPC::PtrArray& rpcs, const ContextPtr& context, int timeoutms)
{
	if (async)
	{
		return async->callByParallel(cb, rpcs, context, timeoutms);
	}

	return false;
}