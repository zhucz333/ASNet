#ifndef __FDEVENT_EPOLL_H__
#define __FDEVENT_EPOLL_H__

#include <unordered_map>
#include "IEvent.h"

class EventEpoll : public IEvent
{
public:
	EventEpoll(int maxfds);
	~EventEpoll();

	virtual int EventInit();
	virtual int EventDestroy();
	virtual int EventAdd(int fd, int events);
	virtual int EventMod(int fd, int events);
	virtual int EventDel(int fd);
	virtual int EventPoll(int millSeconds);
	virtual int EventGetRevents(int index, int& fd, int& revents);

private:
	int m_nMaxfds;
	int m_nEpollfd;
	struct epoll_event* m_ptrEvents;
};

#endif