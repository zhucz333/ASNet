#include "IASNetAPI.h"
#include "ASNetAPIIOCPContext.h"
#include "ASNetAPIIOCP.h"
#include <winsock2.h>
#include <Mswsock.h>
#include <Mstcpip.h>
#include <windows.h>
#include <shared_mutex>

static IASNetAPI* g_ptrNetAPI = new ASNetAPIIOCP();

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

ASNetAPIIOCP::ASNetAPIIOCP()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_bStart = false;
}

ASNetAPIIOCP::~ASNetAPIIOCP()
{
	WSACleanup();
}

bool ASNetAPIIOCP::Init(int workThreadNum, int maxfds)
{
	if (m_bStart) {
		return true;
	}

	m_bStart = true;

	m_nWorkThreadNum = workThreadNum;
	m_nMaxfds = maxfds;

	if (m_nWorkThreadNum <= 0) {
		return false;
	}

	CreateIOCP();

	m_ptrIoService = std::make_shared<MThread>();
	if (m_ptrIoService == nullptr) {
		return false;
	}

	m_ptrIoService->Start(m_nWorkThreadNum);

	m_bStart = true;

	return true;
}

bool ASNetAPIIOCP::Destroy()
{
	if (m_bStart) {
		m_bStart = false;
		for (auto var : m_listThreads) {
			var->join();
		}

		if (m_hIOCompletionPort != NULL && m_hIOCompletionPort != INVALID_HANDLE_VALUE) {
			CloseHandle(m_hIOCompletionPort);
			m_hIOCompletionPort = NULL;
		}
	}
	
	if (m_ptrIoService) {
		m_ptrIoService->Stop();
		m_ptrIoService = NULL;
	}


	{
		std::unique_lock<std::shared_mutex> lg(m_mapMutex);
		m_mapClient.clear();
	}

	return true;
}

int ASNetAPIIOCP::CreateIOCP()
{
	// 建立完成端口
	if (m_hIOCompletionPort != NULL) return 0;

	m_hIOCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (NULL == m_hIOCompletionPort) {
		return -1;
	}

	for (int i = 0; i < m_nWorkThreadNum; i++) {
		m_listThreads.push_back(new std::thread(&ASNetAPIIOCP::WorkerThread, this, i));
	}

	return 0;
}

int ASNetAPIIOCP::CreateClient(IASNetAPIClientSPI* cspi, IASNetAPIPacketHelper* helper, const std::string& strLocalIp, unsigned short uLocalPort, ASNetAPIClientType type)
{
	SOCKET socketID = INVALID_SOCKET;

	if (type == ASNETAPI_TCP_CLIENT) {
		DWORD dwBytes = 0;
		tcp_keepalive setting;
		tcp_keepalive ret;
		ZeroMemory(&setting, sizeof(tcp_keepalive));
		ZeroMemory(&ret, sizeof(tcp_keepalive));
		setting.onoff = 1;
		setting.keepalivetime = 5000;
		setting.keepaliveinterval = 2000;

		socketID = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (WSAIoctl(socketID, SIO_KEEPALIVE_VALS, &setting, sizeof(setting), &ret, sizeof(ret), &dwBytes, NULL, NULL) != 0) {
			return INVALID_SOCKET;
		}
	} else if (type == ASNETAPI_UDP_CLIENT) {
		socketID = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	if (socketID == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}

	SOCKADDR_IN localAddr;
	ZeroMemory((char *)&localAddr, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(uLocalPort);
	if (0 == strLocalIp.length()) {
		localAddr.sin_addr.s_addr = htonl(ADDR_ANY);
	} else {
		localAddr.sin_addr.s_addr = inet_addr(strLocalIp.c_str());
	}

	if (SOCKET_ERROR == bind(socketID, (struct sockaddr *) &localAddr, sizeof(localAddr))) {
		return INVALID_SOCKET;
	}

	if (NULL == CreateIoCompletionPort((HANDLE)socketID, m_hIOCompletionPort, (DWORD)socketID, 0)) {
		return INVALID_SOCKET;
	}

	std::shared_ptr<Connection> ptrConn = std::make_shared<Connection>(cspi, helper, m_ptrIoService, socketID, strLocalIp, uLocalPort, type);

	{
		std::unique_lock<std::shared_mutex> ul(m_mapMutex);
		m_mapClient[socketID] = ptrConn;
	}

	return socketID;
}

bool ASNetAPIIOCP::DestroyClient(int nSocketId, std::string& errMsg)
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

	{
		std::unique_lock<std::shared_mutex> ul(m_mapMutex);
		m_mapClient[nSocketId] = nullptr;
	}

	return true;
}

bool ASNetAPIIOCP::Listen(int nSocketId, int backlog, std::string& errMsg)
{
	std::shared_ptr<Connection> ptrConn = nullptr;

	{
		std::shared_lock<std::shared_mutex> sl(m_mapMutex);
		ptrConn = m_mapClient[nSocketId];
	}

	if (false == ptrConn->Listen(backlog, errMsg)) {
		ptrConn->OnError(errMsg);
		return false;
	}

	return true;
}

bool ASNetAPIIOCP::Connect(int nSocketId, const std::string& strServerIp, unsigned short uServerPort, std::string& errMsg)
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

	if (CONNECTION_CONNECT_FAILED == state) {
		ptrConn->OnError(errMsg);
		return false;
	}

	return true;
}

bool ASNetAPIIOCP::Send(int nSocketId, const char* pData, unsigned int nLen, std::string& errMsg)
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

	if (ret) {
		ptrConn->OnError(errMsg);
		return false;
	}

	return true;
}

bool ASNetAPIIOCP::Close(int nSocketId)
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

int ASNetAPIIOCP::WorkerThread(int nThreadID)
{
	OVERLAPPED *overlapped = NULL;
	void *unused = NULL;
	ASNetAPIIOCPContext *ctx = NULL;
	DWORD dwBytesTransfered = 0;
	BOOL bRet = 0;
	std::shared_ptr<Connection> ptrConn = nullptr;

	while (m_bStart) {
		bRet = GetQueuedCompletionStatus(m_hIOCompletionPort, &dwBytesTransfered, (PULONG_PTR)&unused, &overlapped, 1000);
		if (NULL == overlapped) {
			continue;
		}
		
		ctx = CONTAINING_RECORD(overlapped, ASNetAPIIOCPContext, m_Overlapped);

		{
			std::shared_lock<std::shared_mutex> sl(m_mapMutex);
			ptrConn = m_mapClient[ctx->m_nSocketId];
		}
		if (ptrConn == nullptr) {
			ptrConn->OnError("IOCP recv msg, But Connection does not exist：" + std::to_string((int)*socket));
			continue;
		}

		if (FALSE == bRet) {
			int error = WSAGetLastError();

			if (ERROR_NETNAME_DELETED == error) {
				ptrConn->OnClose();
			} else {
				ptrConn->OnError("GetQueuedCompletionStatus Error, Error Code:" + std::to_string(error));
			}

			continue;
		}

		if ((0 == dwBytesTransfered) && (IOCP_RECV_POSTED == ctx->m_eOpType)) {
			ptrConn->OnClose();
			continue;
		}

		std::string in, svrIp, cltIp, errMsg;
		std::shared_ptr<Connection> conn = NULL;

		switch (ctx->m_eOpType) {
		case IOCP_ACCEPT_POSTED:
			conn = ptrConn->OnAccept(ctx->m_nAcceptSocketId, ctx->m_wsaBuffer.buf);

			if (conn && CreateIoCompletionPort((HANDLE)ctx->m_nAcceptSocketId, m_hIOCompletionPort, (DWORD)ctx->m_nAcceptSocketId, 0)) {
				{
					std::unique_lock<std::shared_mutex> ul(m_mapMutex);
					m_mapClient[ctx->m_nAcceptSocketId] = conn;
				}

				if (false == conn->PostRecv(errMsg)) {
					conn->OnError(errMsg);
				}
			}

			delete[]ctx->m_wsaBuffer.buf;
			break;
		case IOCP_CONNECT_POSTED:
			ptrConn->OnConnect();
			break;
		case IOCP_RECV_POSTED:
			in.append(ctx->m_wsaBuffer.buf, dwBytesTransfered);
			ptrConn->OnRecv(in);

			delete []ctx->m_wsaBuffer.buf;
			break;
		case IOCP_SEND_POSTED:
			ptrConn->OnSend(ctx->m_wsaBuffer.buf, ctx->m_wsaBuffer.len);
			break;
		case IOCP_CLOSESOCKET_POSTED:
			ptrConn->OnClose();
			break;
		default:
			break;
		}

		// release ctx
		delete ctx;
	}

	return 0;
}