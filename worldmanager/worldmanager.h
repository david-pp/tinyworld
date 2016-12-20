#ifndef _WORLDMANAGER_SERVER_H
#define _WORLDMANAGER_SERVER_H

#include "app.h"

class WorldManager : public NetApp
{
public:
	WorldManager();

	virtual bool init();
	virtual void fini();

	static WorldManager* instance() 
	{
		static WorldManager worldm;
		return &worldm;
	}

private:
	ProtobufMsgDispatcher dispatcher_;
};


#endif // _WORLDMANAGER_SERVER_H
