
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include "acceptor.h"
#include "mylogger.h"
#include "protos/account.pb.h"

static LoggerPtr logger(Logger::getLogger("server"));

boost::mt19937 gen;                                     
boost::uniform_int<>dist(1,6);
boost::variate_generator<boost::mt19937&,boost::uniform_int<> >die(gen,dist);

void onRegisterWithNamePassword(Cmd::Account::RegisterWithNamePasswordRequest* msg, NetAsio::ConnectionPtr conn)
{
	boost::this_thread::sleep(boost::posix_time::millisec(die()));
	//LOG4CXX_INFO(logger, "client:" << conn);
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());

	Cmd::Account::RegisterWithNamePasswordReply reply;
	reply.set_retcode(0);
	reply.set_accid(1111);
	conn->send(reply);
}

int main(int argc, char* argv[])
{
	BasicConfigurator::configure();

	std::string host = "0.0.0.0";
	std::string port = "8888";
	size_t iothreads = 2;
	size_t workerthreads = 1;

	// Check command line arguments.
	if (argc < 2)
	{
		std::cerr << "Usage: http_server <address> <port> <iothreads> <workerthreads>\n";
		std::cerr << "  For IPv4, try:\n";
		std::cerr << "    receiver 0.0.0.0 80 1 .\n";
		std::cerr << "  For IPv6, try:\n";
		std::cerr << "    receiver 0::0 80 1 .\n";
		return 1;
	}

	if (argc > 1) host = argv[1];
	if (argc > 2) port = argv[2];
	if (argc > 3) iothreads = atol(argv[3]);
	if (argc > 4) workerthreads = atol(argv[4]);

	NetAsio::IOServicePool io_pool(iothreads);
	io_pool.init();

	NetAsio::WorkerPool work_pool(workerthreads);
	work_pool.init();

	ProtobufMsgDispatcher dispatcher;
	dispatcher.bind<Cmd::Account::RegisterWithNamePasswordRequest, NetAsio::ConnectionPtr>(onRegisterWithNamePassword);

	NetAsio::Acceptor acceptor(host, port, io_pool, work_pool, dispatcher);
	acceptor.listen();

	io_pool.run();
	work_pool.run();

	return 0;
}
