#include "Connection.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <shared_mutex>

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

	if (0 != listen(m_nSocketId, backlog)) {
		errMsg.append("Call listen() error, ErrorCode = " + std::to_string(errno) + ", error info : " + strerror(errno));
		return false;
	}

	m_eConnetionState = CONNECTION_LISTENING;

	return true;
}

ConnectionState Connection::Connect(const std::string& strServerIp, unsigned short uServerPort, std::string &errMsg)
{
	if (ASNETAPI_UDP_CLIENT == m_clientType && 0 == strServerIp.length()) {
		m_strRemoteIP = strServerIp;
		m_nRemotePort = uServerPort;

		return CONNECTION_CONNECTED;
	}

	if (m_eConnetionState == CONNECTION_CONNECTED || m_eConnetionState == CONNECTION_CONNECTING) {
		return m_eConnetionState;
	}

	m_strRemoteIP = strServerIp;
	m_nRemotePort = uServerPort;
	m_bSending = false;
	{
		std::unique_lock<std::mutex> lock(m_mutexSendData);
		m_listSendData.clear();
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(strServerIp.c_str());
	serv_addr.sin_port = htons(uServerPort);

	int ret = connect(m_nSocketId, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (0 == ret) {
		m_eConnetionState = CONNECTION_CONNECTED;
	} else if (ret < 0 && errno == EINPROGRESS) {
		m_eConnetionState = CONNECTION_CONNECTING;
	} else {
		errMsg.append(strerror(errno));
		m_eConnetionState = CONNECTION_CONNECT_FAILED;
	}

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

	{
		std::unique_lock<std::mutex> lock(m_mutexSendData);
		m_listSendData.push_back(std::string(pData, nLen));
	}
	return nLen;
}

bool Connection::Close()
{
	close(m_nSocketId);
	m_eConnetionState = CONNECTION_CLOSED;

	return true;
}

std::shared_ptr<Connection> Connection::OnAccept(int socketId, const char* in)
{
	struct sockaddr_in cltAddr;
	socklen_t addrLen = sizeof(cltAddr);

	int connfd = accept(m_nSocketId, (struct sockaddr*)&cltAddr, &addrLen);
	int flag = fcntl(connfd, F_GETFL, 0);
	if (flag < 0 || fcntl(connfd, F_SETFL, flag | O_NONBLOCK) < 0) {
		perror("fcntl setnoblock failed!");
		return NULL;
	}
	
	std::string strClientIp(inet_ntoa(cltAddr.sin_addr));
	unsigned short uClientPort = ntohs(cltAddr.sin_port);
	m_ptrCSPI->OnAccept(m_nSocketId, connfd, strClientIp, uClientPort);

	std::shared_ptr<Connection> ptrConn = std::make_shared<Connection>(m_ptrCSPI, m_ptrHelper, m_ptrThread, connfd, m_strLocalIP, m_nLocalPort, ASNETAPI_TCP_CLIENT, CONNECTION_CONNECTED);
	return ptrConn;
}

void Connection::OnConnect()
{
	if (m_ptrCSPI) {
		m_ptrCSPI->OnConnect(m_nSocketId, m_strRemoteIP, m_nRemotePort);
	}
}

void Connection::OnSend(char *data, int len)
{
	if (m_eConnetionState == CONNECTION_CONNECTING) {
		m_eConnetionState = CONNECTION_CONNECTED;

		m_ptrCSPI->OnConnect(m_nSocketId, m_strRemoteIP, m_nRemotePort);
		return;
	}

	if (m_bSending) {
		return;
	}

	m_bSending = true;

	std::unique_lock <std::mutex> lock(m_mutexSendData);
	while (!m_listSendData.empty()) {
		auto var = m_listSendData.begin();
		int ret = write(m_nSocketId, var->c_str(), var->size());

		if (var->size() == ret) {
			m_ptrCSPI->OnSend(m_nSocketId, var->c_str(), var->size());
			m_listSendData.pop_front();
			continue;
		} else if (ret > 0 && ret < var->size()) {
			std::string data = var->substr(ret);
			m_listSendData.pop_front();
			m_listSendData.push_front(data);
			continue;
		} else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			continue;
		} else {
			OnError("SEND error, errno: " + std::to_string(errno) + ", " + strerror(errno));
			break;
		}
	}

	m_bSending = false;
}

void Connection::OnRecv(const std::string& in)
{
	{
		std::unique_lock <std::mutex> lock(m_mutexRecvData);
		m_listRecvData.push_back(in);
	}

	m_strand.Post(std::bind(&Connection::DoRecv, this));
}

void Connection::OnError(const std::string& errMsg)
{
	if (m_ptrCSPI) {
		m_ptrCSPI->OnError(m_nSocketId, errMsg);
	}
}

void Connection::OnClose()
{
	if (m_eConnetionState != CONNECTION_CLOSED) {
		close(m_nSocketId);
		m_eConnetionState = CONNECTION_CLOSED;
	}

	if (m_ptrCSPI) {
		m_ptrCSPI->OnClose(m_nSocketId);
	}
}

void Connection::DoRecv()
{
	if (m_bDoRecving || m_ptrCSPI == NULL) {
		return;
	}

	m_bDoRecving = true;

	do {
		std::unique_lock <std::mutex> lock(m_mutexRecvData);
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
	} while(1);

	m_bDoRecving = false;
}
