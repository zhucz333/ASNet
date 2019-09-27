#ifndef __IEVENT_H__
#define __IEVENT_H__

#define BV(x) (1<<x) 

#define FD_EVENT_IN		BV(0)
#define FD_EVENT_PRI	BV(1)
#define FD_EVENT_OUT	BV(2)
#define FD_EVENT_ERR	BV(3)
#define FD_EVENT_HUP	BV(4)
#define FD_EVENT_NVAL	BV(5)

class IEvent
{
public:
	virtual int EventInit() = 0;
	virtual int EventDestroy() = 0;
	virtual int EventAdd(int fd, int events) = 0;
	virtual int EventMod(int fd, int events) = 0;
	virtual int EventDel(int fd) = 0;
	virtual int EventPoll(int millSeconds) = 0;
	virtual int EventGetRevents(int index, int& fd, int& revents) = 0;
public:
	virtual ~IEvent() {};
};
#endif
