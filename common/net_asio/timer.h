#ifndef _NETASIO_TIMER_H
#define _NETASIO_TIMER_H

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "net.h"

NAMESPACE_NETASIO_BEGIN

struct Timer
{
public:
    Timer(boost::asio::io_service& ios, int ms) 
        : t_(ios, boost::posix_time::millisec(ms))
    {
        ms_ = ms;
        t_.async_wait(boost::bind(&Timer::triggered, this,
          boost::asio::placeholders::error));
    }

    ~Timer() {}

    void triggered(const boost::system::error_code& e)
    {
        run();

        t_.expires_at(t_.expires_at() + boost::posix_time::millisec(ms_));
        t_.async_wait(boost::bind(&Timer::triggered, this, 
          boost::asio::placeholders::error));
    }

    virtual void run() = 0;

protected:
    int ms_;
    boost::asio::deadline_timer t_;
};

NAMESPACE_NETASIO_END

#endif // _NETASIO_TIMER_H
