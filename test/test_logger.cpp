#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <cstdlib>
#include <unistd.h>

#include "../common/mylogger.h"

//
// !!! 中文无法显示，请重新编译log4cxx:
// ./configure --with-charset=utf-8 --with-logchar=utf-8
//

//using namespace log4cxx;

/////////////////////////////////////////////////

LoggerPtr logger(Logger::getLogger("MyApp"));

/////////////////////////////////////////////////

namespace com {
	namespace foo {
		class Bar {
			static LoggerPtr logger;

			public:
				void doIt()
				{
					LOG4CXX_DEBUG(logger, "Did it again!")
				}
		};

		LoggerPtr Bar::logger(Logger::getLogger("com.foo.bar"));
	}
}

/////////////////////////////////////////////////

struct Feature1 {
	static log4cxx::LoggerPtr logger;

	void doIt()
	{
		LOG4CXX_DEBUG(logger, "第一个功能");
	}

	struct Feature1_1
	{
		static log4cxx::LoggerPtr logger;

		void doIt()
		{
			LOG4CXX_TRACE(logger, "第一个功能的子功能")
		}
	};
};

LoggerPtr Feature1::logger(Logger::getLogger("X"));
LoggerPtr Feature1::Feature1_1::logger(Logger::getLogger("X.Y"));

//////////////////////////////////////////////

struct Feature2 {
	static log4cxx::LoggerPtr logger;

	void doIt()
	{
		LOG4CXX_DEBUG(logger, "第二个功能");
	}
};

LoggerPtr Feature2::logger(Logger::getLogger("feature2"));
//////////////////////////////////////////////


#include "log4cxx/logstring.h"
#include "log4cxx/net/smtpappender.h"
#include "log4cxx/ttcclayout.h"
#include "log4cxx/helpers/pool.h"


//
// ./configure --with-SMTP=libesmtp
//
void initSMTP()
{
	using namespace log4cxx;
	using namespace log4cxx::net;
	using namespace log4cxx::helpers;

	SMTPAppenderPtr appender(new SMTPAppender());
	appender->setSMTPHost(LOG4CXX_STR("smtp.qq.com"));
	appender->setSMTPPort(25);
	appender->setSMTPUsername("user@qq.com");
	appender->setSMTPPassword("passwd");
	appender->setTo(LOG4CXX_STR("user@gmail.com"));
	appender->setFrom(LOG4CXX_STR("logger@qq.com"));
	appender->setSubject("日志");
	appender->setLayout(new TTCCLayout());
	Pool p;
	appender->activateOptions(p);
	
	Logger::getLogger("feature2")->addAppender(appender);
}


int main(int argc, char **argv)
{
	int result = EXIT_SUCCESS;

	if (argc > 1)
	{
		//PropertyConfigurator::configureAndWatch(argv[1], 3000);
		PropertyConfigurator::configure(argv[1]);
		BasicConfigurator::resetConfiguration();
		PropertyConfigurator::configure(argv[1]);
	}
	else
	{
		BasicConfigurator::configure();
	}

	//initSMTP();

	com::foo::Bar bar;
	Feature1 f1;
	Feature1::Feature1_1 f11;
	Feature2 f2;

	LOG4CXX_INFO(logger, "Entering application." << 1)

	while (true)
	{
		bar.doIt();
		f1.doIt();
		f11.doIt();
		f2.doIt();
		sleep(1);
	}

	LOG4CXX_INFO(logger, "Exiting application." << 2)

	return result;
}
