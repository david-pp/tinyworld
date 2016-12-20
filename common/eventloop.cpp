#include "eventloop.h"

////////////////////////////////////////////////////////////////////////

static void idle_cb (struct ev_loop *loop, ev_idle *w, int revents)
{
	IdleEvent* ev = (IdleEvent*)w->data;
	ev->called();
}

static void timer_cb (struct ev_loop *loop, ev_timer *w, int revents)
{
	TimerEvent* ev = (TimerEvent*)w->data;
	ev->called();
}

////////////////////////////////////////////////////////////////////////

EventLoop::EventLoop(bool default_)
{
	if (default_)
		evloop_ = ev_default_loop(0);
	else
		evloop_ = ev_loop_new(EVFLAG_AUTO);
}

void EventLoop::loop()
{
	if (!evloop_) return;

	ev_loop(evloop_, 0);
}

void EventLoop::regIdle(IdleEvent* ev)
{
	if (evloop_ && ev)
	{
		ev->ev_.data = ev;
		ev_idle_init (&ev->ev_, idle_cb);
		ev_idle_start (evloop_, &ev->ev_);
	}
}

void EventLoop::regTimer(TimerEvent* ev, ev_tstamp repeat, ev_tstamp after)
{
	if (evloop_ && ev)
	{
		ev->ev_.data = ev;
		ev_timer_init (&ev->ev_, timer_cb, after, repeat);
		ev_timer_start (evloop_, &ev->ev_);
	}
}
