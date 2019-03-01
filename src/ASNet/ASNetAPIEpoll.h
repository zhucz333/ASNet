#ifndef __ASNETAPI_EPOOL_H__
#define __ASNETAPI_EPOOL_H__

#include "IASNetAPI.h"
#include "IASNetAPIClientSPI.h"
#include "IEvent.h"
#include "Connection.h"
#include "MThread.h"
#include "Strand.h"

#include <unordered_map>
#include <atomic>
#include <shared_mutex>

class ASNetAPIEpoll : public IASNetAPI
{
public:
	ASNetAPIEpoll();
	~ASNetAPIEpoll();

public:
	virtual bool Init(int workThreadNum, int maxfds);
	virtual bool Destroy();
	virtual int CreateClient(IASNetAPIClientSPI* spi, IASNetAPIPacketHelper* helper, const std::string& strLocalIp, unsigned short uLocalPort, ASNetAPIClientType type);
	virtual bool DestroyClient(int nSocketId, std::string& errMsg);
	virtual bool Listen(int nSocketId, int backlog, std::string& errMsg);
	virtual bool Connect(int nSocketId, const std::string& strServerIp, unsigned short uServerPort, std::string& errMsg);
	virtual bool Send(int nSocketId, const char* pData, unsigned int nLen, std::string& errMsg);
	virtual bool Close(int nSocketId);

private:
	void DoJob();
	void DoIOPoll();

	void OnConnect(int nSocketId);
	void OnRecv(int nSocketID, const std::string &inData);
	void OnSend(int nSocketID);
	void OnError(int nSocketID, const std::string &errMsg);
	void OnClose(int nSocketID);
	

	int ReadAll(int nSocketID, std::string &buff);

private:
	int m_nWorkThreadNum;
	int m_nMaxfds;

	std::atomic<bool> m_bStart;
	std::shared_ptr<IEvent> m_ptrFDEvent;
	std::shared_mutex m_mapMutex;
	std::unordered_map<int, std::shared_ptr<Connection>> m_mapClient;

	std::shared_ptr<MThread> m_ptrIoService;
	std::thread *m_ptrPollThread;
};
#endif
