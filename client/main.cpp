#include "client.h"
#include <boost/tokenizer.hpp>
#include "mylogger.h"
#include "protos/account.pb.h"

static LoggerPtr logger(Logger::getLogger("myclient"));


void onMsg1(Cmd::Account::RegisterWithDeviceIDReply* proto, 
    NetAsio::ConnectionPtr conn)
{
    LOG4CXX_INFO(logger, proto->GetTypeName() << ":" << proto->DebugString());
}

void onMsg2(Cmd::Account::LoginWithNamePasswdRep* proto, 
    NetAsio::ConnectionPtr conn)
{
    LOG4CXX_INFO(logger, proto->GetTypeName() << ":" << proto->DebugString());
}


boost::asio::io_service io_service;
ProtobufMsgDispatcher dispatcher;

int main (int argc, const char* argv[])
{
    BasicConfigurator::configure();

    if (argc < 3)
    {
        std::cout << "Usage: client <server> <point>\n";
        std::cout << "Example:\n";
        std::cout << "  ./client 127.0.0.1 8888\n";
        return 1;
    }

    dispatcher.bind<Cmd::Account::RegisterWithDeviceIDReply, NetAsio::ConnectionPtr>(onMsg1);
    dispatcher.bind<Cmd::Account::LoginWithNamePasswdRep, NetAsio::ConnectionPtr>(onMsg2);

    try {

        NetAsio::ClientPtr client(new NetAsio::Client(argv[1], argv[2], io_service, io_service, dispatcher));
        client->start();
        boost::shared_ptr<boost::thread> thread(new boost::thread(
          boost::bind(&boost::asio::io_service::run, &io_service)));

        while(true)
        {
            if (client->isConnected())
            {
                
                std::string input;
                std::cout << "cmd> ";

                std::getline(std::cin, input);

                std::vector<std::string> cmd;
                typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                boost::char_separator<char> sep(" \t,");
                tokenizer tok(input, sep);
                for(tokenizer::iterator beg=tok.begin();  beg!=tok.end(); ++beg)
                {
                    cmd.push_back(*beg);
                }

                if (cmd.size())
                {
                    LOG4CXX_INFO(logger, cmd[0]);
                    if ("regdevice" == cmd[0])
                    {
                        if (cmd.size() >= 2)
                        {
                            Cmd::Account::RegisterWithDeviceIDRequest msg;
                            msg.set_device_type("IOS");
                            msg.set_device_id(cmd[1]);
                            client->send(msg);
                            LOG4CXX_INFO(logger, "regdevice : " << cmd[1]);
                        }
                        else
                        {
                            LOG4CXX_ERROR(logger, "regdevice deviceid");
                        }
                    }
                    else if ("login" == cmd[0])
                    {
                        if (cmd.size() >= 3)
                        {
                            Cmd::Account::LoginWithNamePasswdReq req;
                            req.set_username(cmd[1]);
                            req.set_passwd(cmd[2]);
                            client->send(req);
                            LOG4CXX_INFO(logger, "login : " << cmd[1] << "," << cmd[2]);
                        }
                    }
                }  
            }
        }

        thread->join();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n"; 
    }

    return 0;
}
