#ifndef _COMMON_APP_H
#define _COMMON_APP_H

#include <string>
#include <iostream>
#include "zmq.hpp"
#include "tinyworld.h"
#include "message.h"

class AppBase
{
public:
	AppBase(const char* name);
	virtual ~AppBase();

	void run(int argc, const char* argv[]);
	
	void stop();
	void pause();
	void resume();

	virtual bool init();
	virtual void fini();
	virtual void mainloop() = 0;

	static AppBase* instance();

	enum RunState
	{
		kRunState_Closed   = 0,
		kRunState_Running  = 1,
		kRunState_Paused   = 2,
		kRunState_Stoped = 3,
	};

	RunState getRunState() { return runstate_; }

	bool isRunning() { return kRunState_Running == runstate_; }
	bool isStoped() { return kRunState_Stoped == runstate_; }

private:
	RunState runstate_;

	std::string name_;
};

class NetApp : public AppBase
{
public:
	NetApp(const char* name) : AppBase(name)
	{
		context_ = NULL;
		socket_client_ = NULL;
		socket_server_ = NULL;
		memset(pollitems_, 0, sizeof(pollitems_));
	}

	virtual ~NetApp()
	{
	}

	virtual bool init();
	virtual void fini();
	virtual void mainloop();

	virtual void onIdle();


	bool bind(const char* address);
	bool connect(const char* address);

	bool setID(const std::string id);
	void dump (zmq::socket_t& socket, const char* desc);

	template <typename MSG>
	void sendToClient(const std::string& id, const MSG& msg)
	{
		zmq::message_t idmsg(id.size());
		memcpy(idmsg.data(), id.data(), id.size());
		socket_server_->send (idmsg, ZMQ_SNDMORE);
		sendmsg(msg, *socket_server_);
	}

	template <typename MSG>
	void sendToServer(const std::string& id, const MSG& msg)
	{
		zmq::message_t idmsg(id.size());
		memcpy(idmsg.data(), id.data(), id.size());
		socket_client_->send (idmsg, ZMQ_SNDMORE);
		
		sendmsg(msg, *socket_client_);
	}

	void recvFromClient(zmq::message_t& data, zmq::message_t& clientid);
	void recvFromServer(zmq::message_t& data, zmq::message_t& serverid);

protected:
	zmq::context_t* context_;
	zmq::socket_t*  socket_client_;
	zmq::socket_t*  socket_server_;
	zmq_pollitem_t pollitems_[2];

	std::string id_;
};


enum ServerTypeEnum
{
	kServerType_Invalid = 0, 
	kServerType_WorldServer = 1,
	kServerType_DBServer = 2, 
	kServerType_CellServer = 3,
	kServerType_GateServer = 4,
	kServerType_AppServer = 5,
	kServerType_Machined = 6,
};

class ServerApp : public NetApp
{
public:
	ServerApp(ServerTypeEnum type, const char* name)
		: NetApp(name)
	{
		servertype_ = type;
	}

	virtual ~ServerApp() {}

	bool connectByID(const std::string& sid)
	{
		return true;
	}

	bool connectByType(const ServerTypeEnum type)
	{
		return true;
	}

protected:
	ServerTypeEnum servertype_;

	zmq::context_t* mdsync_;
	zmq::context_t* mdasync_;
};





#endif // _COMMON_APP_H