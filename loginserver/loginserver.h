#ifndef _LOGINSERVER_H
#define _LOGINSERVER_H

#include "app.h"
#include "mydb.h"

class LoginServer : public AppBase
{
public:
	static MySqlConnectionPool* account_dbpool;

public:
	LoginServer();

	static LoginServer* instance() 
	{
		static LoginServer login;
		return &login;
	}

	virtual bool init();
	virtual void run();
	virtual void fini();

	boost::asio::io_service* get_signal_service()
	{
		return worker_pool_ ? &worker_pool_->get_io_service() : NULL;
	}

public:
	///TODO: 临时为之<IP,<端口, 人数>>
	std::map<std::string, std::map<uint32, uint32> > gateusers;

	std::pair<std::string, uint32> getGateAddress()
	{
		if (gateusers.size() && gateusers.begin()->second.size())
		{
			return std::make_pair(gateusers.begin()->first, gateusers.begin()->second.begin()->first);
		}
		return std::make_pair("127.0.0.1", 8888);
	}

protected:
	void createTables();
	bool loadConfig();

private:
	boost::shared_ptr<NetAsio::IOServicePool> io_service_pool_;

	boost::shared_ptr<NetAsio::WorkerPool> worker_pool_;

	/// 对外服务器(客户端连接)
	boost::shared_ptr<NetAsio::Acceptor> server_outter_;

	/// 对内服务器(内部服务器连接)
	boost::shared_ptr<NetAsio::Acceptor> server_inner_;

	/// 用户消息分发器
	ProtobufMsgDispatcher user_msg_dispatcher_;

	/// 服务器消息分发器
	ProtobufMsgDispatcher server_msg_dispatcher_;

	/// 配置
	boost::property_tree::ptree config_;
};

#endif // _LOGINSERVER_H