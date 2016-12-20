#include "app.h"
#include <iomanip>
#include <string>
#include <sstream>

static int s_interrupted = 0;

static void s_signal_handler (int signal_value)
{
    s_interrupted = 1;

    std::cout << "catch the signal : " << signal_value << std::endl;

    AppBase::instance()->stop();
}

static void s_catch_signals (void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

static AppBase* s_app_instance;

AppBase* AppBase::instance()
{
	return s_app_instance;
}

AppBase::AppBase(const char* name)
{
	name_ = name;
	s_app_instance = this;
}

AppBase::~AppBase()
{
	s_app_instance = NULL;
}

void AppBase::run(int argc, const char* argv[])
{
	s_catch_signals();

	if (init())
	{
		while (!isStoped())
		{
			this->mainloop();
		}
	}

	fini();
}

void AppBase::stop()
{
	runstate_ = kRunState_Stoped;
}


void AppBase::pause()
{

}

void AppBase::resume()
{

}

bool AppBase::init()
{
	return true;
}

void AppBase::fini()
{
}



bool NetApp::init()
{
	if (!AppBase::init())
		return false;

	try {
		context_ = new zmq::context_t(1);
		socket_server_ = new zmq::socket_t(*context_, ZMQ_ROUTER);
		socket_client_ = new zmq::socket_t(*context_, ZMQ_ROUTER);

		pollitems_[0].socket = *socket_server_;
		pollitems_[0].events = ZMQ_POLLIN;

		pollitems_[1].socket = *socket_client_;
		pollitems_[1].events = ZMQ_POLLIN;
	} 
	catch (zmq::error_t& e) {
		std::cout << "failed:" << e.what() << std::endl;
		return false;
	}

	return true;
}

void NetApp::fini()
{
	delete socket_server_;
	delete socket_client_;
	delete context_;
}

void NetApp::mainloop()
{
	try {
		int rc = zmq_poll (pollitems_, 2,  100);
		if (-1 != rc)
		{
			if (pollitems_[0].revents & ZMQ_POLLIN)
			{
				//dump(*socket_server_, "[server]"); return;

        		zmq::message_t idmsg;
        		socket_server_->recv(&idmsg);

        		zmq::message_t datamsg;
        		socket_server_->recv(&datamsg);

        		this->recvFromClient(datamsg, idmsg);
	            //socket_server_->send (idmsg, ZMQ_SNDMORE);
	            //s_send (*socket_server_, id_ + " | This is the reply from...");
			}

			if (pollitems_[1].revents & ZMQ_POLLIN)
			{
				//dump(*socket_client_, "[client]"); return;

				zmq::message_t idmsg;
        		socket_client_->recv(&idmsg);

        		zmq::message_t datamsg;
        		socket_client_->recv(&datamsg);

        		this->recvFromServer(datamsg, idmsg);
			}

			this->onIdle();
		}
	}
	catch(zmq::error_t& e) {
		std::cout << "W: interrupt received, proceeding…" << std::endl;
	}
}

void NetApp::onIdle()
{
	std::cout << "idle ... " << std::endl;
}

void NetApp::recvFromClient(zmq::message_t& msg, zmq::message_t& clientid)
{
	std::string id(static_cast<char*>(clientid.data()), clientid.size());
	MsgDispatcher<0>::instance().dispatchWithArg(msg.data(), msg.size(), &id);
}

void NetApp::recvFromServer(zmq::message_t& msg, zmq::message_t& serverid)
{
	std::string id(static_cast<char*>(serverid.data()), serverid.size());
	MsgDispatcher<1>::instance().dispatchWithArg(msg.data(), msg.size(), &id);
}

bool NetApp::bind(const char* address)
{
	try {
		socket_server_->bind(address);
	}
	catch (zmq::error_t& e) {
		std::cout << "failed:" << e.what() << std::endl;
		return false;
	}

	return true;
}

bool NetApp::connect(const char* address)
{
	try {
		socket_client_->connect(address);
	}
	catch (zmq::error_t& e) {
		std::cout << "failed:" << e.what() << std::endl;
		return false;
	}

	return true;
}

bool NetApp::setID(const std::string id)
{
	zmq_setsockopt (*socket_server_, ZMQ_IDENTITY, id.data(), id.size());
	zmq_setsockopt (*socket_client_, ZMQ_IDENTITY, id.data(), id.size());
	id_ = id;
	return true;
}

void NetApp::dump(zmq::socket_t& socket, const char* desc)
{
    std::cout << desc << ":----------------------------------------" << std::endl;

    while (1) {
        //  Process all parts of the message
        zmq::message_t message;
        socket.recv(&message);

        //  Dump the message as text or binary
        int size = message.size();
        std::string data(static_cast<char*>(message.data()), size);

        bool is_text = true;

        int char_nbr;
        unsigned char byte;
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            byte = data [char_nbr];
            if (byte < 32 || byte > 127)
                is_text = false;
        }
        std::cout << desc << ":[" << std::setfill('0') << std::setw(3) << size << "]";
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            if (is_text)
                std::cout << (char)data [char_nbr];
            else
                std::cout << std::setfill('0') << std::setw(2)
                   << std::hex << (unsigned int) data [char_nbr];
        }
        std::cout << std::endl;

        int more = 0;           //  Multipart detection
        size_t more_size = sizeof (more);
        socket.getsockopt (ZMQ_RCVMORE, &more, &more_size);
        if (!more)
            break;              //  Last message part
    }
}