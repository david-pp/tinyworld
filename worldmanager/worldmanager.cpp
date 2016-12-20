#include "worldmanager.h"
#include "mylogger.h"
#include "protos/login.pb.h"
#include "protos/server.pb.h"
#include "url.h"

static LoggerPtr logger(Logger::getLogger("gate"));

void onForwardSync(Cmd::Server::SyncGateUserCount* msg, NetAsio::ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());

	WorldManager::instance()->connector()->broadcastTo(100, *msg);
}

WorldManager::WorldManager() : NetApp("worldmanager", dispatcher_, dispatcher_)
{
	dispatcher_.bind<Cmd::Server::SyncGateUserCount, NetAsio::ConnectionPtr>(onForwardSync);
}

bool WorldManager::init()
{
	return NetApp::init();
}

void WorldManager::fini()
{
	NetApp::fini();
}