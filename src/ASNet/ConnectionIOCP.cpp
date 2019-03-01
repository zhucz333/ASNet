#include "ASNetAPIIOCPContext.h"
#include "Connection.h"
#include "IASNetAPI.h"
#include <Mswsock.h>
#include <Mstcpip.h>

#define MAX_RECV_BUFFER_SIZE 16*1024

Connection::Connection(IASNetAPIClientSPI* cspi, IASNetAPIPacketHelper* helper, const std::shared_ptr<MThread> &service, int socketID, const std::string localIP, unsigned short localPort, ASNetAPIClientType type, ConnectionState state) : m_ptrCSPI(cspi), m_strand(*service), m_ptrThread(service), m_eConnetionState(state)
{
	m_nSocketId = socketID;
	m_strLocalIP = localIP;
	m_nLocalPort = localPort;
	m_strRemoteIP = "";
	m_nRemotePort = 0;
	m_clientType = type;
	m_bSending = false;
	m_bDoRecving = false;
	m_eConnetionReadMachineState = CONNECTION_READ_HEAD;
	m_ptrHelper = helper;
	m_nTotalSize = 0;
	m_strPacket = "";
	m_lpfnConnectEx = NULL;
	m_lpfnAcceptEx = NULL;
	m_lpfnAcceptExSockaddrs = NULL;
}

Connection::~Connection()
{
	if (m_eConnetionState != CONNECTION_CLOSED) {
		Close();
	}
}

bool Connection::Listen(int backlog, std::string& errMsg)
{
	if (NULL == m_ptrCSPI) {
		errMsg.append("Call listen() error: ASNETAPIServerSPI should not be NULL while CreateClient!");
		return false;
	}

	if (SOCKET_ERROR == listen(m_nSocketId, backlog)) {
		errMsg.append("Call listen() error, ErrorCode = " + std::to_string(GetLastError()));
		return false;
	}
	if (NULL == m_lpfnAcceptEx) {
		GUID GuidAcceptEx = WSAID_ACCEPTEX;

		DWORD dwBytes = 0;
		if (SOCKET_ERROR == WSAIoctl(m_nSocketId, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_lpfnAcceptEx, sizeof(m_lpfnAcceptEx), &dwBytes, NULL, NULL)) {
			errMsg.append("WSAIoctl cannot get function AcceptEx, Error Code: " + std::to_string(WSAGetLastError()));
			return false;
		}
	}

	for (int i = 0; i < backlog; i++) {
		int acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == acceptSocket) {
			errMsg.append("Cannot get acceptSocket for AcceptEx(), Error Code: " + std::to_string(WSAGetLastError()));
			return false;
		}

		if (false == PostAcceptEx(acceptSocket, errMsg)) {
			return false;
		}
	}

	return true;
}

bool Connection::PostConnect(std::string& errMsg)
{
	GUID GuidConnectEx = WSAID_CONNECTEX;

	DWORD dwBytes = 0;
	if (NULL == m_lpfnConnectEx) {
		if (SOCKET_ERROR == WSAIoctl(m_nSocketId, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &m_lpfnConnectEx, sizeof(m_lpfnConnectEx), &dwBytes, NULL, NULL)) {
			errMsg.append("WSAIoctl cannot get function ConnectEx, Error Code: " + std::to_string(WSAGetLastError()));
			m_eConnetionState = CONNECTION_UNCONNECTED;
			return false;
		}
	}

	SOCKADDR_IN serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(m_strRemoteIP.c_str());
	serv_addr.sin_port = htons(m_nRemotePort);

	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();
	ctx->m_eOpType = IOCP_CONNECT_POSTED;
	ctx->m_nSocketId = m_nSocketId;

	BOOL ret = m_lpfnConnectEx(m_nSocketId, (sockaddr*)&serv_addr, sizeof(serv_addr), NULL, 0, &dwBytes, &ctx->m_Overlapped);

	if (TRUE == ret) {
		OnConnect();
	}
	else if ((FALSE == ret) && (WSA_IO_PENDING == WSAGetLastError())) {
		m_eConnetionState = CONNECTION_CONNECTING;
	}
	else {
		errMsg.append("Call ConnectEx() error, Error Code: " + std::to_string(WSAGetLastError()));
		m_eConnetionState = CONNECTION_CONNECT_FAILED;
	}

	return true;
}

bool Connection::PostRecv(std::string& errMsg)
{
	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();

	if (NULL == ctx) {
		return false;
	}

	ctx->m_eOpType = IOCP_RECV_POSTED;
	ctx->m_nSocketId = m_nSocketId;
	ctx->m_wsaBuffer.buf = new char[MAX_RECV_BUFFER_SIZE];
	ctx->m_wsaBuffer.len = MAX_RECV_BUFFER_SIZE;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	// 初始化完成后，，投递WSARecv请求
	int ret = WSARecv(m_nSocketId, &ctx->m_wsaBuffer, 1, &dwBytes, &dwFlags, &ctx->m_Overlapped, NULL);

	if ((SOCKET_ERROR == ret) && (WSA_IO_PENDING != WSAGetLastError())) {
		errMsg.append("POST WSARecv() error, Error Code:" + std::to_string(WSAGetLastError()));

		delete[]ctx->m_wsaBuffer.buf;
		delete ctx;

		return false;
	}

	return true;
}

bool Connection::PostAcceptEx(int nAcceptSocketId, std::string& errMsg)
{
	DWORD dwBytes = 0;
	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();
	ctx->m_eOpType = IOCP_ACCEPT_POSTED;
	ctx->m_nSocketId = m_nSocketId;
	ctx->m_nAcceptSocketId = nAcceptSocketId;
	ctx->m_wsaBuffer.buf = new char[MAX_RECV_BUFFER_SIZE];
	ctx->m_wsaBuffer.len = MAX_RECV_BUFFER_SIZE;

	BOOL ret = m_lpfnAcceptEx(m_nSocketId, nAcceptSocketId, ctx->m_wsaBuffer.buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &ctx->m_Overlapped);
	
	if(FALSE == ret && WSA_IO_PENDING != WSAGetLastError()) {
		errMsg.append("Call AcceptEx() failed, ErrorCode : " + std::to_string(WSAGetLastError()));
		return false;
	}
	
	return true;
}

ConnectionState Connection::Connect(const std::string& strServerIp, unsigned short uServerPort, std::string &errMsg)
{
	if (NULL == m_ptrCSPI) {
		errMsg.append("Connection is not a Client, Cannot call Connect(), fd = " + std::to_string(m_nSocketId));
		return CONNECTION_UNCONNECTED;
	}

	if (m_clientType == ASNETAPI_UDP_CLIENT) {
		m_strLocalIP = strServerIp;
		m_nLocalPort = uServerPort;
		OnConnect();

		return CONNECTION_CONNECTED;
	}

	if (m_eConnetionState == CONNECTION_CONNECTED || m_eConnetionState == CONNECTION_CONNECTING) {
		return m_eConnetionState;
	}

	m_strRemoteIP = strServerIp;
	m_nRemotePort = uServerPort;

	PostConnect(errMsg);

	return m_eConnetionState;
}

ConnectionState Connection::GetConnectionState()
{
	return m_eConnetionState;
}

int Connection::GetSocketId()
{
	return m_nSocketId;
}

ASNetAPIClientType Connection::GetClientType()
{
	return m_clientType;
}

int Connection::Send(const char* pData, unsigned int nLen, std::string& errMsg)
{
	if (m_eConnetionState != CONNECTION_CONNECTED) {
		errMsg.append("Connection is not connected, please connect to remote first!");
		return -1;
	}

	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();

	if (NULL == ctx) {
		return -1;
	}

	ctx->m_eOpType = IOCP_SEND_POSTED;
	ctx->m_nSocketId = m_nSocketId;
	ctx->m_wsaBuffer.buf = (char *)pData;
	ctx->m_wsaBuffer.len = nLen;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	int ret = 0;

	if (m_clientType == ASNETAPI_UDP_CLIENT) {
		SOCKADDR_IN remoteAddr;
		memset(&remoteAddr, 0, sizeof(remoteAddr));
		remoteAddr.sin_family = AF_INET;
		remoteAddr.sin_port = htons(m_nLocalPort);
		remoteAddr.sin_addr.s_addr = inet_addr(m_strLocalIP.c_str());
		
		ret = WSASendTo(m_nSocketId, &ctx->m_wsaBuffer, 1, &dwBytes, dwFlags, (SOCKADDR*)&remoteAddr, sizeof(SOCKADDR_IN), &ctx->m_Overlapped, NULL);
	} else if (m_clientType == ASNETAPI_TCP_CLIENT) {
		ret = WSASend(m_nSocketId, &ctx->m_wsaBuffer, 1, &dwBytes, dwFlags, &ctx->m_Overlapped, NULL);
	}

	if ((SOCKET_ERROR == ret) && (WSA_IO_PENDING != WSAGetLastError())) {
		errMsg.append("Send error, Error Code:" + std::to_string(WSAGetLastError()));
		return -1;
	}

	return ret;
}

bool Connection::Close()
{
	if (m_eConnetionState == CONNECTION_CLOSED) {
		return true;
	}

	if (INVALID_SOCKET != m_nSocketId) {
		shutdown(m_nSocketId, SD_BOTH);
		closesocket(m_nSocketId);
		m_nSocketId = INVALID_SOCKET;
	}
	m_eConnetionState = CONNECTION_CLOSED;

	return true;
}

std::shared_ptr<Connection> Connection::OnAccept(int nAcceptSocketId, const char* buff)
{
	if (NULL == m_ptrCSPI) {
		OnError("OnAccept return, but ASNETAPIServerSPI is NULL, fd = " + std::to_string(m_nSocketId));
		return NULL;
	}

	if (NULL == m_lpfnAcceptExSockaddrs) {
		GUID GetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

		DWORD dwBytes = 0;
		if (SOCKET_ERROR == WSAIoctl(m_nSocketId, SIO_GET_EXTENSION_FUNCTION_POINTER, &GetAcceptExSockaddrs, sizeof(GetAcceptExSockaddrs), &m_lpfnAcceptExSockaddrs, sizeof(m_lpfnAcceptExSockaddrs), &dwBytes, NULL, NULL)) {
			OnError("WSAIoctl cannot get function GetAcceptExSockaddrs, Error Code: " + std::to_string(WSAGetLastError()));
			return NULL;
		}
	}

	SOCKADDR_IN *ptrSvrAddr, *ptrCltAddr;
	int svrAddrLen = sizeof(SOCKADDR_IN), cltAddrLen = sizeof(SOCKADDR_IN);
	m_lpfnAcceptExSockaddrs((PVOID)buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&ptrSvrAddr, &svrAddrLen, (LPSOCKADDR*)&ptrCltAddr, &cltAddrLen);
	
	std::string strServerIp(inet_ntoa(ptrSvrAddr->sin_addr)), strClientIp(inet_ntoa(ptrCltAddr->sin_addr));
	unsigned short uServerPort = ntohs(ptrSvrAddr->sin_port), uClientPort = ntohs(ptrCltAddr->sin_port);

	m_ptrCSPI->OnAccept(m_nSocketId, nAcceptSocketId, strClientIp, uClientPort);
	
	//add AcceptEX post
	int acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == acceptSocket) {
		OnError("Cannot get acceptSocket for AcceptEx(), Error Code: " + std::to_string(WSAGetLastError()));
		return NULL;
	}
	std::string errMsg;
	if (false == PostAcceptEx(acceptSocket, errMsg)) {
		OnError(errMsg);
		return NULL;
	}

	//create connection with the nAcceptSocketId
	std::shared_ptr<Connection> ptrConn = std::make_shared<Connection>(m_ptrCSPI, m_ptrHelper, m_ptrThread, nAcceptSocketId, m_strLocalIP, m_nLocalPort, ASNETAPI_TCP_CLIENT, CONNECTION_CONNECTED);
	return ptrConn;
}

void Connection::OnConnect()
{
	if (m_eConnetionState == CONNECTION_CONNECTED) {
		return;
	}
	
	m_eConnetionState = CONNECTION_CONNECTED;

	if (m_ptrCSPI) {
		m_ptrCSPI->OnConnect(m_nSocketId, m_strLocalIP, m_nLocalPort);
	}

	// POST A Receive request
	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();

	if (NULL == ctx) {
		return;
	}

	ctx->m_eOpType = IOCP_RECV_POSTED;
	ctx->m_nSocketId = m_nSocketId;
	ctx->m_wsaBuffer.buf = new char[MAX_RECV_BUFFER_SIZE];
	ctx->m_wsaBuffer.len = MAX_RECV_BUFFER_SIZE;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	// 初始化完成后，，投递WSARecv请求
	int ret = WSARecv(m_nSocketId, &ctx->m_wsaBuffer, 1, &dwBytes, &dwFlags, &ctx->m_Overlapped, NULL);

	if ((SOCKET_ERROR == ret) && (WSA_IO_PENDING != WSAGetLastError())) {
		std::string errMsg;
		OnError(errMsg.append("POST WSARecv() error, Error Code:" + std::to_string(WSAGetLastError())));

		delete []ctx->m_wsaBuffer.buf;
		delete ctx;

		return;
	}
}

void Connection::OnSend(char *data, int length)
{
	m_ptrCSPI->OnSend(m_nSocketId, data, length);
}

void Connection::OnRecv(const std::string& in)
{
	// Add the data to the list
	{
		std::lock_guard <std::mutex> lock(m_mutexRecvData);
		m_listRecvData.push_back(in);
	}

	m_strand.Post(std::bind(&Connection::DoRecv, this));

	// POST A Receive request
	ASNetAPIIOCPContext *ctx = new ASNetAPIIOCPContext();

	if (NULL == ctx) {
		return;
	}

	ctx->m_eOpType = IOCP_RECV_POSTED;
	ctx->m_nSocketId = m_nSocketId;
	ctx->m_wsaBuffer.buf = new char[MAX_RECV_BUFFER_SIZE];
	ctx->m_wsaBuffer.len = MAX_RECV_BUFFER_SIZE;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	// POst WSARecv Request
	int ret = WSARecv(m_nSocketId, &ctx->m_wsaBuffer, 1, &dwBytes, &dwFlags, &ctx->m_Overlapped, NULL);

	if ((SOCKET_ERROR == ret) && (WSA_IO_PENDING != WSAGetLastError())) {
		OnError("POST WSARecv() error, Error Code:" + std::to_string(WSAGetLastError()));

		delete[]ctx->m_wsaBuffer.buf;
		delete ctx;

		return;
	}
}

void Connection::OnError(const std::string& errMsg)
{
	if (m_ptrCSPI) {
		m_ptrCSPI->OnError(m_nSocketId, errMsg);
	}
}

void Connection::OnClose()
{
	if (m_ptrCSPI) {
		m_ptrCSPI->OnClose(m_nSocketId);
	}

	if (m_eConnetionState != CONNECTION_CLOSED) {
		Close();
	}
}

void Connection::DoRecv()
{
	if (m_bDoRecving || m_ptrCSPI == NULL) {
		return;
	}

	m_bDoRecving = true;

	do {
		std::lock_guard <std::mutex> lock(m_mutexRecvData);
		if (m_listRecvData.empty()) {
			break;
		}

		if (!m_ptrHelper) {
			auto var = m_listRecvData.begin();

			m_ptrCSPI->OnRecieve(m_nSocketId, var->c_str(), var->size());
			m_listRecvData.pop_front();
		} else {
			int need = 0;
			m_nHeaderSize = m_ptrHelper->GetPacketHeaderSize();
			if (m_nHeaderSize <= 0) {
				OnError("GetPacketHeaderSize return error, header size = " + std::to_string(m_nHeaderSize));
				break;
			}

			switch (m_eConnetionReadMachineState) {
			case CONNECTION_READ_HEAD:
				if (m_nHeaderSize > m_strPacket.size()) {
					need = m_nHeaderSize - m_strPacket.size();
				}

				for (; !m_listRecvData.empty() && need > 0;) {
					auto iter = m_listRecvData.begin();
					if (iter->size() > need) {
						m_strPacket.append(iter->substr(0, need));
						std::string str = iter->substr(need);
						m_listRecvData.pop_front();
						m_listRecvData.push_front(str);
						need = 0;

						break;
					} else {
						m_strPacket.append(*iter);
						need -= iter->size();
						m_listRecvData.pop_front();
					}
				}

				if (0 == need) {
					m_nTotalSize = m_ptrHelper->GetPacketTotalSize(m_strPacket.c_str(), m_nHeaderSize);
					if (m_nTotalSize < 0) {
						OnError("GetPacketTotalSize return error, total size = " + std::to_string(m_nTotalSize));
						Close();
					}
					m_eConnetionReadMachineState = CONNECTION_READ_BODY;
				}

				break;
			case CONNECTION_READ_BODY:
				if (m_nTotalSize > m_strPacket.size()) {
					need = m_nTotalSize - m_strPacket.size();
				}

				for (; !m_listRecvData.empty() && need > 0; ) {
					auto iter = m_listRecvData.begin();
					if (iter->size() > need) {
						m_strPacket.append(iter->substr(0, need));
						std::string str = iter->substr(need);
						m_listRecvData.pop_front();
						m_listRecvData.push_front(str);
						need = 0;

						break;
					} else {
						m_strPacket.append(*iter);
						need -= iter->size();
						m_listRecvData.pop_front();
					}
				}

				if (0 == need) {
					m_ptrCSPI->OnRecieve(m_nSocketId, m_strPacket.c_str(), m_strPacket.size());

					m_strPacket.clear();
					m_nTotalSize = 0;
					m_eConnetionReadMachineState = CONNECTION_READ_HEAD;
				}
				break;
			default:
				break;
			}
		}
	} while (1);

	m_bDoRecving = false;
}
