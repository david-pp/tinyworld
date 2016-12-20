#ifndef __COMMON_EVENT_LOOP_H
#define __COMMON_EVENT_LOOP_H

#include <ev.h>

class EventLoop;

struct TimerEvent
{
public:
	friend class EventLoop;

	virtual void called() = 0;

private:
	struct ev_timer ev_;
};

struct IdleEvent
{
public:
	friend class EventLoop;

	virtual void called() = 0;

private:
	struct ev_idle ev_;
};

class EventLoop
{
public:
	EventLoop(bool default_= true);

	static EventLoop* instance()
	{
		static EventLoop loop;
		return &loop;
	}

	void regIdle(IdleEvent* ev);
	void regTimer(TimerEvent* ev, ev_tstamp repeat, ev_tstamp after = 0.0);

	void loop();

	struct ev_loop* evLoop() { return evloop_; }

private:
	struct ev_loop* evloop_;
	struct ev_idle  evidle_;
};

#endif // __COMMON_EVENT_LOOP_H