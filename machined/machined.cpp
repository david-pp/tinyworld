//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <iomanip>
#include <string>
#include <sstream>

#include <signal.h>

#include "message.h"
#include "login.pb.h"
#include "app.h"


struct MachinedApp : public NetApp
{
public:
    MachinedApp() : NetApp("machined")
    {
    }

    bool init()
    {
        NetApp::init();
        this->setID("md");
        this->bind("tcp://*:5555");
    }

    void onIdle()
    {

    }

private:
    typedef std::map<std::string, std::string> AppNameMap;

    AppNameMap app_urls_;
    AppNameMap cached_urls_;
};

MachinedApp app;

DefMsgHandler(0, msgcli, LoginRequest, msg)
{
    std::cout <<getArg<std::string>() << ":woha ... LoginRequest" << std::endl;

    msgcli::LoginReply rep;
    rep.set_retcode(msgcli::LoginReply::OKAY);
    app.sendToClient(getArg<std::string>(), rep);
    //app.sendToClient(this->getArg<std::string>(), rep);
    //app.sendToClient(this->getArg<std::string>(), rep);
}

int main (int argc, const char* argv[])
{
    app.run(argc, argv);
}
