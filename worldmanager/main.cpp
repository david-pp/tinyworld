#include "worldmanager.h"

int main (int argc, const char* argv[])
{
	//BasicConfigurator::configure();

	WorldManager::instance()->main(argc, argv);
}