#ifndef __ASNETAPI_IOCP_H__
#define __ASNETAPI_IOCP_H__

#include "IASNetAPI.h"
#include "Connection.h"
#include <atomic>
#include <list>
#include <thread>
#include <unordered_map>
#include <Windows.h>
#include <shared_mutex>

class ASNetAPIIOCP : public IASNetAPI
{
public:
	ASNetAPIIOCP();
	~ASNetAPIIOCP();

public:
	virtual bool Init(int workThreadNum, int maxfds);
	virtual bool Destroy();
	virtual int CreateClient(IASNetAPIClientSPI* cspi, IASNetAPIPacketHelper* helper, const std::string& strLocalIp, unsigned short uLocalPort, ASNetAPIClientType type);
	virtual bool DestroyClient(int nSocketId, std::string& errMsg);
	virtual bool Listen(int nSocketId, int backlog, std::string& errMsg);
	virtual bool Connect(int nSocketId, const std::string& strServerIp, unsigned short uServerPort, std::string& errMsg);
	virtual bool Send(int nSocketId, const char* pData, unsigned int nLen, std::string& errMsg);
	virtual bool SendTo(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIp, unsigned short uRemotePort, std::string &errMsg);
	virtual bool Close(int nSocketId);


private:
	int CreateIOCP();
	int WorkerThread(int nThreadID);

private:
	std::atomic<bool> m_bStart;
	int m_nWorkThreadNum;
	int m_nMaxfds;

	HANDLE m_hIOCompletionPort;
	std::list<std::thread*> m_listThreads;

	std::shared_mutex m_mapMutex;
	std::unordered_map<int, std::shared_ptr<Connection>> m_mapClient;

	std::shared_ptr<MThread> m_ptrIoService;
};

#endif // !__ASNETAPI_IOCP_H__
