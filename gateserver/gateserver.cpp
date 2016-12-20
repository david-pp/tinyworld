#include "gateserver.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "mylogger.h"
#include "protos/login.pb.h"
#include "protos/server.pb.h"
#include "net_asio/timer.h"
#include "url.h"

static LoggerPtr logger(Logger::getLogger("gate"));

void onLogin(Cmd::Login::LoginRequest* msg, NetAsio::ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());

	Cmd::Login::LoginReply reply;
	reply.set_retcode(Cmd::Login::LoginReply::OK);
	conn->send(reply);

	GateServer::instance()->syncInfo2Login();
}

GateServer::GateServer() : NetApp("gateserver", user_msg_dispatcher_, server_msg_dispatcher_)
{
	user_msg_dispatcher_.bind<Cmd::Login::LoginRequest, NetAsio::ConnectionPtr>(onLogin);
}

struct SyncInfo2LoginTimer : public NetAsio::Timer 
{
	SyncInfo2LoginTimer(boost::asio::io_service& ios) 
	  : NetAsio::Timer(ios, 3000) {}

	virtual void run()
	{
		GateServer::instance()->syncInfo2Login();
	}
};


bool GateServer::init()
{
	if (NetApp::init())
	{

		// TODO:临时为之
		SyncInfo2LoginTimer* timer = new SyncInfo2LoginTimer(worker_pool_->get_io_service());
		if (timer)
		{

		}

		return true;
	}

	return false;
}

void GateServer::fini()
{
	NetApp::fini();
}

void GateServer::syncInfo2Login()
{
	LOG4CXX_INFO(logger, "同步状态到登录服务器");

	// 发送到WorldManager
	URL listen;
	if (listen.parse(config_.get<std::string>("listen")))
	{
		Cmd::Server::SyncGateUserCount sync;
		sync.set_gate_ip(listen.host);
		sync.set_gate_port(listen.port);
		sync.set_usercount(acceptor()->size());
		connector()->sendTo(1, sync);
	}
}