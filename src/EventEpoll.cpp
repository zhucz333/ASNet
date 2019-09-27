#include <sys/epoll.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "IEvent.h"
#include "EventEpoll.h"

#define MAXFDS_DEFAULT 1024

EventEpoll::EventEpoll(int maxfds) : m_nMaxfds(maxfds)
{
	if (m_nMaxfds < MAXFDS_DEFAULT) {
		m_nMaxfds = MAXFDS_DEFAULT;
	}

	m_nEpollfd = -1;
	m_ptrEvents = NULL;
};

EventEpoll::~EventEpoll()
{
	EventDestroy();
}

int EventEpoll::EventInit()
{
	m_nEpollfd = epoll_create(m_nMaxfds);
	if (m_nEpollfd < 0) {
		return -1;
	}

	m_ptrEvents = static_cast<struct epoll_event*>(calloc(1, sizeof(struct epoll_event) * m_nMaxfds));
	if (NULL == m_ptrEvents) {
		return -1;
	}
	
	return 0;
}

int EventEpoll::EventDestroy()
{
	if (m_nEpollfd > 0) {
		close(m_nEpollfd);
		m_nEpollfd = -1;
	}

	if (m_ptrEvents) {
		free(m_ptrEvents);
		m_ptrEvents = NULL;
	}

	return 0;
}

int EventEpoll::EventAdd(int fd, int events)
{
	struct epoll_event ep;

	memset(&ep, 0, sizeof(ep));
	if (events & FD_EVENT_IN) ep.events |= EPOLLIN;
	if (events & FD_EVENT_OUT) ep.events |= EPOLLOUT;
	ep.events |= EPOLLERR | EPOLLHUP | EPOLLPRI;
	ep.data.fd = fd;

	if (0 != epoll_ctl(m_nEpollfd, EPOLL_CTL_ADD, fd, &ep)) {
		return -1;
	}

	return 0;
}

int EventEpoll::EventMod(int fd, int events)
{
	struct epoll_event ep;
	memset(&ep, 0, sizeof(ep));

	if (events & FD_EVENT_IN) ep.events |= EPOLLIN;
	if (events & FD_EVENT_OUT) ep.events |= EPOLLOUT;
	ep.events |= EPOLLERR | EPOLLHUP | EPOLLPRI;
	ep.data.fd = fd;

	if (0 != epoll_ctl(m_nEpollfd, EPOLL_CTL_MOD, fd, &ep)) {
		return -1;
	}

	return 0;
}

int EventEpoll::EventDel(int fd)
{
	struct epoll_event ep;
	memset(&ep, 0, sizeof(ep));

	if (0 != epoll_ctl(m_nEpollfd, EPOLL_CTL_DEL, fd, &ep)) {
		return -1;
	}

	return 0;
}

int EventEpoll::EventPoll(int msec)
{
	if (msec <= 0) {
		msec = 1000;
	}

	return epoll_wait(m_nEpollfd, m_ptrEvents, m_nMaxfds, msec);
}

int EventEpoll::EventGetRevents(int index, int& fd, int& revents)
{
	int e = 0;
	if (index < 0 || index >= m_nEpollfd) {
		return -1;
	}

	fd = m_ptrEvents[index].data.fd;
	e = m_ptrEvents[index].events;
	if (e & EPOLLIN) revents |= FD_EVENT_IN;
	if (e & EPOLLOUT) revents |= FD_EVENT_OUT;
	if (e & EPOLLERR) revents |= FD_EVENT_ERR;
	if (e & EPOLLHUP) revents |= FD_EVENT_HUP;
	if (e & EPOLLPRI) revents |= FD_EVENT_PRI;

	return 0;
}
