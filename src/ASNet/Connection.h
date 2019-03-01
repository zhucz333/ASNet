#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <atomic>
#include <memory>
#ifdef _WIN32
#include <Mswsock.h>
#include <Mstcpip.h>
#endif
#include "IASNetAPI.h"
#include "IASNetAPIClientSPI.h"
#include "IASNetAPIPacketHelper.h"
#include "MThread.h"
#include "Strand.h"

enum ConnectionState
{
	CONNECTION_UNCONNECTED = 0,
	CONNECTION_CONNECTING,
	CONNECTION_CONNECTED,
	CONNECTION_CONNECT_FAILED,
	CONNECTION_LISTENING,
	CONNECTION_CLOSED
};

enum ConnectionReadMachineState
{
	CONNECTION_READ_HEAD = 0,
	CONNECTION_READ_BODY
};

class Connection
{
public:
	Connection(IASNetAPIClientSPI* cspi, IASNetAPIPacketHelper* helper, const std::shared_ptr<MThread> &service, int sokcetID, const std::string localIP, unsigned short localPort, ASNetAPIClientType type, ConnectionState state = CONNECTION_UNCONNECTED);
	~Connection();

	bool Listen(int backlog, std::string& errMsg);
	ConnectionState Connect(const std::string& strServerIp, unsigned short uServerPort, std::string &errMsg);
	ConnectionState GetConnectionState();
	ASNetAPIClientType GetClientType();
	int GetSocketId();
	int Send(const char* pData, unsigned int nLen, std::string &errMsg);
	bool Close();

	std::shared_ptr<Connection> OnAccept(int nAcceptSocketId, const char* in);
	void OnConnect();
	void OnSend(char *data = NULL, int len = 0);
	void OnRecv(const std::string& in);
	void OnClose();
	void OnError(const std::string &errMsg);

public:
	void DoRecv();

#ifdef _WIN32
	bool PostConnect(std::string& errMsg);
	bool PostRecv(std::string& errMsg);
	bool PostAcceptEx(int nAcceptSocketId, std::string& errMsg);
#endif

private:
	std::atomic<bool> m_bSending;
	std::atomic<bool> m_bDoRecving;
	int m_nSocketId;
	int m_nLocalPort;
	std::string m_strLocalIP;
	int m_nRemotePort;
	std::string m_strRemoteIP;
	ASNetAPIClientType m_clientType;
	enum ConnectionState m_eConnetionState;
	IASNetAPIClientSPI* m_ptrCSPI;

	IASNetAPIPacketHelper* m_ptrHelper;
	ConnectionReadMachineState m_eConnetionReadMachineState;
	int m_nHeaderSize;
	int m_nTotalSize;
	std::string m_strPacket;

	std::mutex m_mutexSendData;
	std::list<std::string> m_listSendData;

	std::shared_ptr<MThread> m_ptrThread;
	Strand m_strand;
	std::mutex m_mutexRecvData;
	std::list<std::string> m_listRecvData;

#ifdef _WIN32
	LPFN_CONNECTEX m_lpfnConnectEx;
	LPFN_ACCEPTEX m_lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnAcceptExSockaddrs;
#endif
};

#endif
