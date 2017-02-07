
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "mylogger.h"
#include "mydb.h"
#include "object2db.h"

LoggerPtr logger(Logger::getLogger("dataserver"));

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage:" << argv[0]
                  << " create | drop | update" << std::endl;
        return 1;
    }

    BasicConfigurator::configure();

    LOG4CXX_DEBUG(logger, "dataserver 启动")

    MySqlConnectionPool::instance()->setServerAddress("mysql://david:123456@127.0.0.1/tinyworld?maxconn=5");
    MySqlConnectionPool::instance()->createAll();


    return 0;
}
