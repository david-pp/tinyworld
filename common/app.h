#ifndef _NET_ASIO_APP_H
#define _NET_ASIO_APP_H

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include "net_asio/acceptor.h"
#include "net_asio/connector.h"
#include "mylogger.h"

//
// 各种服务的基类
//
class AppBase
{
public:
	AppBase(const std::string& name);
	virtual ~AppBase();

	// 单件实例
	static AppBase* instance();

	
	// 应用入口
	void main(int argc, const char* argv[]);
	
	//
	// init - 初始化
	// run  - 主循环
	// fini - 应用结束时释放各种资源
	// 
	virtual bool init();
	virtual void run() = 0;
	virtual void fini();

	//
	// 当前应用的状态
	//
	enum RunState
	{
		kRunState_Closed   = 0,
		kRunState_Running  = 1,
		kRunState_Paused   = 2,
		kRunState_Stoped   = 3,
	};

	RunState getRunState() { return runstate_; }
	bool isRunning() { return kRunState_Running == runstate_; }
	bool isStoped() { return kRunState_Stoped == runstate_; }

	// 信号处理的IOSERVICE
	boost::asio::io_service* get_signal_service() { return NULL; }

protected:
	void handle_stop();

	/// 运行状态
	RunState runstate_;

	/// 应用名称
	std::string name_;

	/// 应用ID
	uint32 id_;

	/// 信号处理
	boost::shared_ptr<boost::asio::signal_set> signals_;
};


//
// 网络节点: 
//   1> 同时扮演服务器和客户端
//   2> 可以接受TCP连接，也可以主动连接其他服务
//
class NetApp : public AppBase
{
public:
	NetApp(const std::string& name,
		ProtobufMsgDispatcher& server_dispatcher,
		ProtobufMsgDispatcher& client_dispatcher,
		const std::string& configfilename = "config.xml");

	virtual bool init();
	virtual void run();
	virtual void fini();

	bool loadConfig();

	std::string makeNodeName(const std::string& name);

	bool sendTo(uint id);
	bool broadcastTo(uint type);
	bool broadcastToAll();

	boost::asio::io_service* get_signal_service()
	{
		return worker_pool_ ? &worker_pool_->get_io_service() : NULL;
	}

	/// TODO:临时用法
	boost::shared_ptr<NetAsio::Acceptor> acceptor() { return server_; }
	boost::shared_ptr<NetAsio::Connector> connector() { return clients_; }
	
protected:
	boost::shared_ptr<NetAsio::IOServicePool> io_service_pool_;

	boost::shared_ptr<NetAsio::WorkerPool> worker_pool_;

	/// 扮演服务器
	boost::shared_ptr<NetAsio::Acceptor> server_;
	ProtobufMsgDispatcher& server_dispatcher_;

	/// 扮演客户端
	boost::shared_ptr<NetAsio::Connector> clients_;
	ProtobufMsgDispatcher& client_dispatcher_;

	/// 配置
	std::string config_filename_;
	boost::property_tree::ptree config_;
};


#endif // _NET_ASIO_APP_H