#include "IASNetAPIClientSPI.h"
#include "ASNetAPIEpoll.h"
#include "EventEpoll.h"
#include "MThread.h"
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static IASNetAPI* g_ptrNetAPI = new ASNetAPIEpoll();

IASNetAPI* IASNetAPI::getInstance()
{
	return g_ptrNetAPI;
}

void IASNetAPI::removeInstance()
{
	if (g_ptrNetAPI) {
		delete g_ptrNetAPI;
		g_ptrNetAPI = NULL;
	}
}

ASNetAPIEpoll::ASNetAPIEpoll()
{
	m_bStart = false;
}

ASNetAPIEpoll::~ASNetAPIEpoll()
{
	
}

bool ASNetAPIEpoll::Init(int workThreadNum, int maxfds)
{
	if (m_bStart) {
		return true;
	}
	
	m_nWorkThreadNum = workThreadNum;
	m_nMaxfds = maxfds;

	if (m_nWorkThreadNum <= 0) {
		return false;
	}

	m_ptrIoService = std::make_shared<MThread>();
	if (m_ptrIoService == nullptr) {
		return false;
	}

	m_ptrIoService->Start(m_nWorkThreadNum);

	m_ptrFDEvent = std::make_shared<EventEpoll>(m_nMaxfds);
	if (m_ptrFDEvent == nullptr) {
		return false;
	}
	m_ptrFDEvent->EventInit();
		
	m_bStart = true;
		
	m_ptrPollThread = new std::thread(std::bind(&ASNetAPIEpoll::DoIOPoll, this));
	
	return true;
}

void ASNetAPIEpoll::DoIOPoll()
{
	while (m_bStart) {
		int num = m_ptrFDEvent->EventPoll(100);
		for (int i = 0; i < num; i++){
			int fd = 0, events = 0;
			m_ptrFDEvent->EventGetRevents(i, fd, events);
				
			if (events & FD_EVENT_ERR || events & FD_EVENT_HUP) {
				OnClose(fd);
			} else if (events & FD_EVENT_OUT) {
				OnSend(fd);
			} else if (events & FD_EVENT_IN) {
				std::string out = "", ip = "";
				unsigned short port = 0;
				if (ReadAll(fd, out, ip, port) > 0) {
					OnRecv(fd, out, ip, port);
				} else {
					OnClose(fd);
				}
			} 
		}
	}
}

int ASNetAPIEpoll::CreateClient(IASNetAPIClientSPI* spi, IASNetAPIPacketHelper* helper = NULL, const std::string& strLocalIp = "", unsigned short uLocalPort = 0, ASNetAPIClientType type = ASNETAPI_TCP_CLIENT)
{
	int socketID = -1;
	if (type == ASNETAPI_TCP_CLIENT) {
		socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		int optVal = 1;
		if (setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
			perror("setsockopt() failed!");
			return -1;
		}

	} else if (type == ASNETAPI_UDP_CLIENT) {
		socketID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (socketID < 0) {
		perror("Call socket() failed!");
		return -1;
	}
	
	int nOptval = 1;
	if (setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptval, sizeof(nOptval)) < 0) {
		return -1;
	}

	struct sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(uLocalPort);
	if (0 == strLocalIp.length()) {
		localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		localAddr.sin_addr.s_addr = inet_addr(strLocalIp.c_str());
	}

	if (0 != bind(socketID, (struct sockaddr *) &localAddr, sizeof(localAddr))) {
		perror("Call bind() failed!");
		close(socketID);
		return -1;
	}

	int flag = fcntl(socketID, F_GETFL, 0);
	if (flag < 0 || fcntl(socketID, F_SETFL, flag | O_NONBLOCK) < 0) {
		perror("fcntl setnoblock failed!");
		return -1;
	}

	std::shared_ptr<Connection> ptrConn = std::make_shared<Connection>(spi, helper, m_ptrIoService, socketID, strLocalIp, uLocalPort, type);
	
	{
		std::unique_lock<std::shared_mutex> ul(m_mapMutex);
		m_mapClient[socketID] = ptrConn;
	}

	return socketID;
}

bool ASNetAPIEpoll::Listen(int nSocketId, int backlog, std::string& errMsg)
{
	std::shared_ptr<Connection> ptrConn = nullptr;

	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}
	
	if (ptrConn == nullptr) {
		errMsg.append("socketID is not valid, please Call CreateClient() first!");
		return false;
	}
	
	if (false == ptrConn->Listen(backlog, errMsg)) {
		ptrConn->OnError(errMsg);
		return false;
	}

	if (m_ptrFDEvent->EventAdd(nSocketId, FD_EVENT_IN) < 0) {
		errMsg.append("ADD FD_EVENT_IN to EPOLL error, errno:" + std::to_string(errno) + ", error info: " + strerror(errno));
		return false;
	}

	return true;
}

bool ASNetAPIEpoll::Connect(int nSocketId, const std::string& strServerIp, unsigned short uServerPort, std::string& errMsg)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (ptrConn == nullptr) {
		errMsg.append("socketID is not valid, please Call CreateClient() first!");
		return false;
	}

	ConnectionState state = ptrConn->Connect(strServerIp, uServerPort, errMsg);
		
	if (CONNECTION_CONNECTED == state) {
		OnConnect(nSocketId);
	} else if (CONNECTION_CONNECTING == state) {
		if(m_ptrFDEvent->EventAdd(nSocketId, FD_EVENT_OUT) < 0) {
			errMsg.append("ADD FD_EVENT_OUT to EPOLL error, errno:" + std::to_string(errno) + ", error info" + strerror(errno));
			return false;
		}
	} else {
		return false;
	}

	return true;
}

bool ASNetAPIEpoll::Send(int nSocketId, const char* pData, unsigned int nLen, std::string& errMsg)
{
	if (nSocketId < 0 || pData == NULL) return false;
		
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (ptrConn == nullptr) {
		errMsg.append("socketID is not valid!");
		return false;
	}
	
	int ret = ptrConn->Send(pData, nLen, errMsg);
	if (ret != nLen) {
		return false;
	}

	m_ptrFDEvent->EventMod(nSocketId, FD_EVENT_OUT | FD_EVENT_IN);

	return true;
}



bool ASNetAPIEpoll::SendTo(int nSocketId, const char * pData, unsigned int nLen, const std::string & strRemoteIp, unsigned short uRemotePort, std::string & errMsg)
{
	if (nSocketId < 0 || pData == NULL) return false;

	std::shared_ptr<Connection> ptrConn = nullptr;

	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (ptrConn == nullptr) {
		errMsg.append("socketID is not valid, please Call CreateClient() first! socketId = " + std::to_string(nSocketId));
		return false;
	}

	int ret = ptrConn->SendTo(pData, nLen, strRemoteIp, uRemotePort, errMsg);

	if (ret) {
		ptrConn->OnError(errMsg);
		return false;
	}

	return true;
}
	
void ASNetAPIEpoll::OnConnect(int nSocketID)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketID];
	}

	if (ptrConn == nullptr) {
		return;
	}

	if (ASNETAPI_UDP_CLIENT == ptrConn->GetClientType()) {
		m_ptrFDEvent->EventAdd(nSocketID, FD_EVENT_OUT | FD_EVENT_IN);
	} else {
		m_ptrFDEvent->EventMod(nSocketID, FD_EVENT_OUT | FD_EVENT_IN);
	}

	m_ptrIoService->Post(std::bind(&Connection::OnConnect, ptrConn.get()));
}

void ASNetAPIEpoll::OnSend(int nSocketID)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketID];
	}

	if (ptrConn == nullptr) {
		return;
	}

	m_ptrFDEvent->EventMod(nSocketID, FD_EVENT_IN);

	m_ptrIoService->Post(std::bind(&Connection::OnSend, ptrConn.get(), nullptr, 0));
}

void ASNetAPIEpoll::OnRecv(int nSocketID, const std::string& inData, const std::string& strRemoteIp, unsigned short nRemotePort)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketID];
	}

	if (ptrConn == nullptr) {
		return;
	}

	// Call OnRecv Orderly
	ptrConn->OnRecv(inData, strRemoteIp, nRemotePort);
}

void ASNetAPIEpoll::OnClose(int nSocketID)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketID];
	}

	if (ptrConn == nullptr) {
		return;
	}

	if (CONNECTION_LISTENING != ptrConn->GetConnectionState()) {
		m_ptrIoService->Post(std::bind(&Connection::OnClose, ptrConn.get()));
		m_ptrFDEvent->EventDel(nSocketID);

		return;
	}

	auto ptrAcceptConn = ptrConn->OnAccept(0, NULL);
	if (ptrAcceptConn) {
		{
			std::unique_lock<std::shared_mutex> ul(m_mapMutex);
			m_mapClient[ptrAcceptConn->GetSocketId()] = ptrAcceptConn;;
		}
		m_ptrFDEvent->EventAdd(ptrAcceptConn->GetSocketId(), FD_EVENT_IN);
	}

	return;
}

void ASNetAPIEpoll::OnError(int nSocketID, const std::string& errMsg)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketID];
	}
	if (ptrConn == nullptr) {
		return;
	}

	m_ptrIoService->Post(std::bind(&Connection::OnError, ptrConn.get(), errMsg));
	m_ptrFDEvent->EventDel(nSocketID);
}

int ASNetAPIEpoll::ReadAll(int nSocketID, std::string& out, std::string& strRemoteIp, unsigned short& nRemotePort)
{
	char buff[MAX_RECV_BUFFER_SIZE] = {};
	int nleft = 0, nread = 0;
	struct sockaddr_in remoteAddr = {0};
	int addrLen = sizeof(remoteAddr);

	memset(buff, 0, sizeof(buff));
	nleft = sizeof(buff);
	nread = 0;
	
	// TODO:为了适配UDP的读取，此处的recv效率偏低，只读一次
	do {
		nread = recvfrom(nSocketID, buff, nleft, 0, (sockaddr*)&remoteAddr, &addrLen);
		if (nread < 0) {
			if (errno == EINTR) {
				continue;
			}
			else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			else {
				return -1;
			}
		}
		else if (0 == nread) {
			break;
		}

		out.append(buff, nread);
		strRemoteIp = inet_ntoa(remoteAddr.sin_addr);
		nRemotePort = ntohs(remoteAddr.sin_port);

		break;
	} while (1);

	return out.size();
}

bool ASNetAPIEpoll::Close(int nSocketId)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (ptrConn == nullptr) {
		return false;
	}

	return ptrConn->Close();
}

bool ASNetAPIEpoll::DestroyClient(int nSocketId, std::string& errMsg)
{
	std::shared_ptr<Connection> ptrConn = nullptr;
	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (ptrConn == nullptr) {
		errMsg.append("socketID is not valid!");
		return false;
	}

	ptrConn->Close();
	m_ptrFDEvent->EventDel(nSocketId);

	{
		std::unique_lock<std::shared_mutex> ul(m_mapMutex);
		m_mapClient[nSocketId] = nullptr;
	}

	return true;
}

bool ASNetAPIEpoll::Destroy()
{
	if (m_bStart) {
		m_bStart = false;
		m_ptrPollThread->join();
		m_ptrFDEvent->EventDestroy();
	}

	if (m_ptrIoService) {
		m_ptrIoService->Stop();
	}

	{
		std::unique_lock<std::shared_mutex> ul(m_mapMutex);
		m_mapClient.clear();
	}

	return true;
}
