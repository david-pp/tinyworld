#include "loginserver.h"

int main (int argc, const char* argv[])
{
	LoginServer::instance()->main(argc, argv);
}