#include "app.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "url.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("app"));

static AppBase* s_app_instance;

/////////////////////////////////////////////////

AppBase* AppBase::instance()
{
	return s_app_instance;
}

AppBase::AppBase(const std::string& name)
{
	runstate_ = kRunState_Closed;
	name_ = name;
	s_app_instance = this;
	id_ = 0;
}

AppBase::~AppBase()
{
	s_app_instance = NULL;
}

void AppBase::main(int argc, const char* argv[])
{
	if (argc > 1)
		id_ = atol(argv[1]);

	if (init())
	{
		while (!isStoped())
		{
			this->run();
		}
	}

	fini();
}

bool AppBase::init()
{
	if (get_signal_service())
	{
		signals_.reset(new boost::asio::signal_set(*get_signal_service()));
		// Register to handle the signals that indicate when the server should exit.
		// It is safe to register for the same signal multiple times in a program,
		// provided all registration for the specified signal is made through Asio.
		signals_->add(SIGINT);
		signals_->add(SIGTERM);
		#if defined(SIGQUIT)
		signals_->add(SIGQUIT);
		#endif // defined(SIGQUIT)
		signals_->async_wait(boost::bind(&AppBase::handle_stop, this));
	}
	return true;
}

void AppBase::fini()
{
}

void AppBase::handle_stop()
{
	LOG4CXX_INFO(logger, "NetApp::stoped");
	this->fini();
	runstate_ = kRunState_Stoped;
}


/////////////////////////////////////////////////


NetApp::NetApp(const std::string& name, 
	ProtobufMsgDispatcher& server_dispatcher,
	ProtobufMsgDispatcher& client_dispatcher,const std::string& configfilename)
        : AppBase(name), 
          server_dispatcher_(server_dispatcher),
          client_dispatcher_(client_dispatcher),
          config_filename_(configfilename)
{
}

bool NetApp::init()
{
	BasicConfigurator::configure();
	//PropertyConfigurator::configure(name_ + ".properties");


	//daemon(1, 1);

	LOG4CXX_INFO(logger, "run as daemon");

	if (!loadConfig())
		return false;

	id_ = config_.get("<xmlattr>.id", 0);

	const size_t iothreads = config_.get("iothreads", 1);
	const size_t workerthreads = config_.get("workerthreads", 1);

	io_service_pool_.reset(new NetAsio::IOServicePool(iothreads));
	if (!io_service_pool_ || !io_service_pool_->init())
	{
		LOG4CXX_ERROR(logger, "NetApp::init io_service_pool_ init failed");
		return false;
	}

	worker_pool_.reset(new NetAsio::WorkerPool(workerthreads));
	if (!worker_pool_ || !worker_pool_->init())
	{
		LOG4CXX_ERROR(logger, "NetApp::init worker_pool_ init failed");
		return false;
	}

	URL listen;
	if (!listen.parse(config_.get<std::string>("listen")))
		return false;

	server_.reset(new NetAsio::Acceptor(listen.host, 
		boost::lexical_cast<std::string>(listen.port),
		*io_service_pool_,
		*worker_pool_, 
		server_dispatcher_));
	if (server_)
	{
		server_->listen();
	}

	clients_.reset(new NetAsio::Connector(
		*io_service_pool_, 
		*worker_pool_, 
		client_dispatcher_));

	if (clients_)
	{
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, config_)
		{
			if (v.first == "connect")
			{
				URL url;
				if (url.parse(v.second.data()))
				{
					std::cout << url.make() << std::endl;

					clients_->connect(
						boost::lexical_cast<uint32>(url.path),
						boost::lexical_cast<uint32>(url.query["type"]),
						url.host,
						boost::lexical_cast<std::string>(url.port),
						1000);
				}
			}
		}
	}

	AppBase::init();

	return true;
}

void NetApp::run()
{
	if (io_service_pool_)
		io_service_pool_->run();

	if (worker_pool_)
		worker_pool_->run();
}

void NetApp::fini()
{
	if (server_)
		server_->close();

	if (clients_)
		clients_->close();

	if (io_service_pool_)
		io_service_pool_->fini();
	if (worker_pool_)
		worker_pool_->fini();
}

bool NetApp::loadConfig()
{
	if (config_filename_.empty())
		return false;

	try
	{
		boost::property_tree::xml_parser::read_xml(config_filename_, config_);
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, 
			config_.get_child("config"))
		{
			//std::cout << v.first << std::endl;
			//std::cout << v.second.get("<xmlattr>.id", 0ul)  << std::endl;
			if (v.first == name_)
			{
				if (id_ > 0 && v.second.get("<xmlattr>.id", 0ul) == id_)
				{
					config_ = v.second;

					//std::cout << config_.get<std::string>("listen") << std::endl;
					return true;
				}
			}
		}

		return false;
	}
	catch (std::exception& e)
	{
		return false;
	}
}

std::string NetApp::makeNodeName(const std::string& name)
{
	std::string nodename = "config.";
	nodename += name_;
	nodename += ".";
	nodename += name;
	return nodename;
}
