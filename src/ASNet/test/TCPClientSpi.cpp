#include <iostream>
#include <string>
#include "TCPClientSpi.h"
#include "IASNetAPI.h"

TCPClientSpi::TCPClientSpi()
{
	m_sendCount = 0;
	m_ptrNetAPI = IASNetAPI::getInstance();
	if (m_ptrNetAPI) {
		m_ptrNetAPI->Init();
	}
}

TCPClientSpi::~TCPClientSpi()
{
	if (m_ptrNetAPI) {
		m_ptrNetAPI->Destroy();
	}
}

int TCPClientSpi::Run(std::string ip, int port)
{
	m_strRemoteIP = ip;
	m_nRemotePort = port;

	return Connect();
}

int TCPClientSpi::Connect()
{
	std::string errMsg;
	m_nSocketID = m_ptrNetAPI->CreateClient(this);
	std::cout << "fd:" << m_nSocketID << std::endl; 
	if (false == m_ptrNetAPI->Connect(m_nSocketID, m_strRemoteIP.c_str(), m_nRemotePort, errMsg)) {
		std::cout << "Call connect to " << m_strRemoteIP.c_str() << ":" << m_nRemotePort << " ERROR! error:" << errMsg.c_str() << std::endl;
		return -1;
	}
	std::cout << "Call connect to " << m_strRemoteIP.c_str() << ":" << m_nRemotePort << " OK, wait connect result!" << std::endl;

	return 0;
}
int TCPClientSpi::Send(int m_nSocketID)
{
	if (++m_sendCount > 3) {
		if (m_ptrNetAPI->Close(m_nSocketID)) {
			std::cout << "Call close OK " << std::endl;
		} else {
			std::cout << "Call close failed " << std::endl;
		}
		return 0;
	}
	std::string msg("Hello World! ++ ");
	msg = msg + std::to_string(m_sendCount);

	std::string errMsg;
	if (false == m_ptrNetAPI->Send(m_nSocketID, msg.c_str(), msg.size(), errMsg)) {
		std::cout << "Call Send to " << m_strRemoteIP.c_str() << ":" << m_nRemotePort << " ERROR! error:" << errMsg.c_str() << std::endl;
		return 0;
	}
	std::cout << "Call Send to " << m_strRemoteIP.c_str() << ":" << m_nRemotePort << " OK, wait Send Result!" << std::endl;

	return 0;
}
void TCPClientSpi::OnAccept(int nListenSocketId, int nNewSocketId, const std::string& strRemoteIP, unsigned short uRemotePort)
{
	return;
}


void TCPClientSpi::OnConnect(int nSocketId, const std::string& strRemoteIP, unsigned short uRemotePort)
{
	std::cout << "connect to " << strRemoteIP.c_str() << ":" << uRemotePort << " OK" << std::endl;
	Send(nSocketId);
}

void TCPClientSpi::OnRecieve(int nSocketId, const char* pData, unsigned int nLen)
{
	std::cout << "RECV msg:" << pData << ", data len:"<< nLen << std::endl;
	Send(nSocketId);
}

void TCPClientSpi::OnSend(int nSocketId, const char* pData, unsigned int nLen)
{
	std::cout << "send msg:" << pData << ", data len:" << nLen << " OK, wait ECHO msg!" << std::endl;
}

void TCPClientSpi::OnClose(int nSocketId)
{
	std::string errMsg;
	std::cout << "RECV OnClose MSG, Reconnect!" << std::endl;
	if (false == m_ptrNetAPI->DestroyClient(nSocketId, errMsg)) {
		std::cout << "DestroyClient Error, msg:" << errMsg.c_str() << std::endl;
	} else {
		std::cout << "DestroyClient OK" << std::endl;
	}
}

void TCPClientSpi::OnError(int nSocketId, const std::string& strErrorInfo)
{
	std::cout << "RECV error:" << strErrorInfo.c_str() << "call Close the connection" << std::endl;
	m_ptrNetAPI->Close(nSocketId);
}

int TCPClientSpi::GetPacketHeaderSize()
{
	return 0;
}

int TCPClientSpi::GetPacketTotalSize(const char* header, int length)
{
	return 0;
}

