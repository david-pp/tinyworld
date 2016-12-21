
#include "mylogger.h"

LoggerPtr logger(Logger::getLogger("dataserver"));

int main(int argc, const char* argv[])
{
    BasicConfigurator::configure();

    LOG4CXX_DEBUG(logger, "dataserver 启动")
}
