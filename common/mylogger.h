#ifndef __COMMON_MY_LOGGER_H
#define __COMMON_MY_LOGGER_H

#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/propertyconfigurator.h"

//////////////////////////////////////////////////////////
//
// logger using log4cxx
//
// ./configure --with-charset=utf-8 --with-logchar=utf-8
//
/////////////////////////////////////////////////////////

using log4cxx::LoggerPtr;
using log4cxx::Logger;
using log4cxx::BasicConfigurator;
using log4cxx::PropertyConfigurator;

// BasicConfigurator::configure();
// PropertyConfigurator::configure(filename);
// PropertyConfigurator::configureAndWatch(filename);


// LOG4CXX_TRACE
// LOG4CXX_DEBUG
// LOG4CXX_INFO
// LOG4CXX_WARN
// LOG4CXX_ERROR
// LOG4CXX_FATAL

// LOG4CXX_ASSERT



#endif // __COMMON_MY_LOGGER_H