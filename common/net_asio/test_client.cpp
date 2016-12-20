#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include "client.h"
#include "connector.h"
#include "timer.h"
#include "mylogger.h"
#include "protos/account.pb.h"

static LoggerPtr logger(Logger::getLogger("client"));

boost::asio::io_service io_service;
ProtobufMsgDispatcher dispatcher;
int clientnum = 1;
int sleepms = 1000;


struct Print : public NetAsio::Timer
{
	Print(boost::asio::io_service& ios, int ms) : NetAsio::Timer(ios, ms){}

	void run()
	{
		std::cout << "timer.." << std::endl;
	}
};

void onMsg(Cmd::Account::RegisterWithNamePasswordReply* msg, NetAsio::ConnectionPtr conn)
{
	LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " << msg->GetTypeName() << ":\n" << msg->DebugString());
}

void clientThread(char* argv[]) 
{
	size_t msgcount = 0;

	try
	{
		NetAsio::ClientPtr client(new NetAsio::Client(argv[1], argv[2], io_service, io_service, dispatcher, 3000));
		client->start();
		while(true)
		{
			if (client->isConnected())
			{
				Cmd::Account::RegisterWithNamePasswordRequest request;
				request.set_username("david" + boost::lexical_cast<std::string>(++msgcount));
				request.set_passwd("123456");
				LOG4CXX_INFO(logger, "发送内容:\n" << request.DebugString());
				client->send(request);
			}
			else
			{
				LOG4CXX_ERROR(logger, "Connection is not Ready ...");
			}

			boost::this_thread::sleep(boost::posix_time::millisec(sleepms));
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
}

#if 0
void clientThread2(char* argv[]) 
{
	//size_t msgcount = 0;

	try
	{
		NetAsio::ClientManager clientmgr(2, 1, dispatcher);
		clientmgr.connect(1, 1, argv[1], argv[2], 1000);
		clientmgr.init();
		clientmgr.run();
		clientmgr.fini();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
}

#endif 

int main(int argc, char* argv[])
{
	BasicConfigurator::configure();

	if (argc < 3)
	{
		std::cout << "Usage: client <server> <port> <clientnum> <sleepms>\n";
		std::cout << "Example:\n";	
		std::cout << "./client 127.0.0.1 8888\n";
		return 1;
	}

	if (argc == 4)
		clientnum = atol(argv[3]);
	if (argc == 5)
		sleepms = atol(argv[4]);

	try
	{
		dispatcher.bind<Cmd::Account::RegisterWithNamePasswordReply, NetAsio::ConnectionPtr>(onMsg);

		boost::thread_group threads;
		for ( int i = 0; i < clientnum; ++i)
			threads.create_thread(boost::bind(clientThread, argv));

		Print print(io_service, 1000);

		io_service.run();
		threads.join_all();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	return 0;
}
