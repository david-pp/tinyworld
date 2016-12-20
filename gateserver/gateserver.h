#ifndef _COMMON_GATESERVER_H
#define _COMMON_GATESERVER_H

#include "app.h"

class GateServer : public NetApp
{
public:
	GateServer();

	virtual bool init();
	virtual void fini();

	static GateServer* instance() 
	{
		static GateServer gate;
		return &gate;
	}

	void syncInfo2Login();

private:
	ProtobufMsgDispatcher user_msg_dispatcher_;
	ProtobufMsgDispatcher server_msg_dispatcher_;
};


#endif // _COMMON_GATESERVER_H 