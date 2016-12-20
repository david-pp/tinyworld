#include "loginserver.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include "protos/account.pb.h"
#include "protos/server.pb.h"
#include "mylogger.h"
#include "url.h"

static LoggerPtr logger(Logger::getLogger("login"));

MySqlConnectionPool* LoginServer::account_dbpool = NULL;

#include "account.h"

// 收到网关同步的消息
void onSyncFromGate(Cmd::Server::SyncGateUserCount* proto, 
	NetAsio::ConnectionPtr conn)
{
	LoginServer::instance()->gateusers[proto->gate_ip()][proto->gate_port()] = proto->usercount();

	LOG4CXX_INFO(logger, "收到网关信息:" <<  proto->gate_ip() << ":" << proto->gate_port() << ", 连接数量:" << proto->usercount());
}

LoginServer::LoginServer() : AppBase("loginserver")
{
	user_msg_dispatcher_.bind<Cmd::Account::RegisterWithDeviceIDRequest, NetAsio::ConnectionPtr>(
		onRegisterWithDeviceID);
	user_msg_dispatcher_.bind<Cmd::Account::RegisterWithNamePasswordRequest, NetAsio::ConnectionPtr>(
		onRegisterWithNamePassword);
	user_msg_dispatcher_.bind<Cmd::Account::RegisterBindNamePasswordRequest, NetAsio::ConnectionPtr>(
		onRegisterBindNamePassword);
	user_msg_dispatcher_.bind<Cmd::Account::LoginWithNamePasswdReq, NetAsio::ConnectionPtr>(
		onLoginWithNamePasswd);

	server_msg_dispatcher_.bind<Cmd::Server::SyncGateUserCount,
		NetAsio::ConnectionPtr>(
		onSyncFromGate);
}

bool LoginServer::loadConfig()
{
	try
	{
		boost::property_tree::xml_parser::read_xml("config.xml", config_);
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, 
			config_.get_child("config"))
		{
			//std::cout << v.first << std::endl;
			//std::cout << v.second.get("<xmlattr>.id", 0ul)  << std::endl;
			if (v.first == name_)
			{
				config_ = v.second;

				//std::cout << config_.get<std::string>("listen") << std::endl;
				return true;
			}
		}

		return false;
	}
	catch (std::exception& e)
	{
		return false;
	}
}


bool LoginServer::init()
{
	BasicConfigurator::configure();

	if (!loadConfig())
	{
		LOG4CXX_ERROR(logger, "加载配置config.xml失败");
		return false;
	}

	LoginServer::account_dbpool = new MySqlConnectionPool();
	if (account_dbpool)
	{
		LoginServer::account_dbpool->setServerAddress(config_.get<std::string>("mysql"));
		LoginServer::account_dbpool->createAll();
		createTables();
	}

	id_ = config_.get("<xmlattr>.id", 0);

	const size_t iothreads = config_.get("iothreads", 1);
	const size_t workerthreads = config_.get("workerthreads", 1);

	io_service_pool_.reset(new NetAsio::IOServicePool(iothreads));
	if (!io_service_pool_ || !io_service_pool_->init())
	{
		LOG4CXX_ERROR(logger, "LoginServer::init io_service_pool_ init failed");
		return false;
	}

	worker_pool_.reset(new NetAsio::WorkerPool(workerthreads));
	if (!worker_pool_ || !worker_pool_->init())
	{
		LOG4CXX_ERROR(logger, "LoginServer::init worker_pool_ init failed");
		return false;
	}

	URL listen_outter;
	URL listen_inner;
	if (!listen_outter.parse(config_.get<std::string>("listen_outter")))
		return false;
	if (!listen_inner.parse(config_.get<std::string>("listen_inner")))
		return false;

	server_outter_.reset(new NetAsio::Acceptor(listen_outter.host, 
		boost::lexical_cast<std::string>(listen_outter.port),
		*io_service_pool_,
		*worker_pool_, 
		user_msg_dispatcher_));
	if (server_outter_)
	{
		server_outter_->listen();
	}

	server_inner_.reset(new NetAsio::Acceptor(listen_inner.host, 
		boost::lexical_cast<std::string>(listen_inner.port),
		*io_service_pool_,
		*worker_pool_, 
		server_msg_dispatcher_));
	if (server_inner_)
	{
		server_inner_->listen();
	}

	LOG4CXX_INFO(logger, "LoginServer初始化成功");

	AppBase::init();

	return true;
}

void LoginServer::fini()
{
	if (server_outter_)
		server_outter_->close();

	if (server_inner_)
		server_inner_->close();

	if (io_service_pool_)
		io_service_pool_->fini();
	if (worker_pool_)
		worker_pool_->fini();

	LOG4CXX_INFO(logger, "LoginServer::fini");
}


void LoginServer::run()
{
	if (io_service_pool_)
		io_service_pool_->run();

	if (worker_pool_)
		worker_pool_->run();
}

void LoginServer::createTables()
{
	ScopedMySqlConnection mysql(LoginServer::account_dbpool);
	if (mysql)
	{
		try {
			mysqlpp::Query query = mysql->query();
			query.reset();
			query << "CREATE TABLE IF NOT EXISTS `account` (" 
				  << "`accid` INT(10) UNSIGNED NOT NULL auto_increment,"
				  << "`username` VARCHAR(64) NOT NULL default '',"
				  << "`passwd` VARCHAR(128) NOT NULL default '',"
				  << "`device_type` VARCHAR(64) NOT NULL default '',"
				  << "`device_id` VARCHAR(64) NOT NULL default '',"
				  << "PRIMARY KEY (`accid`)"
				  << ") ENGINE = InnoDB;";

			query.execute();
		}
		catch (const mysqlpp::Exception& er) {
			LOG4CXX_ERROR(logger, "初始化表格失败:" << er.what());
		}
	}
}