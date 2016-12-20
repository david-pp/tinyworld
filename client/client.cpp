#include "client.h"
#include <boost/tokenizer.hpp>
#include "mylogger.h"
#include "protos/account.pb.h"

static LoggerPtr logger(Logger::getLogger("myclient"));
