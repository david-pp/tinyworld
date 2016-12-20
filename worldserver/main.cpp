#include "worldserver.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "app.h"


ProtobufMsgDispatcher dispatcher;

int main (int argc, const char* argv[])
{
	//BasicConfigurator::configure();
	
	NetApp app("worldserver", dispatcher, dispatcher);

	app.main(argc, argv);
}