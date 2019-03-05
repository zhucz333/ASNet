#include <iostream>
#include <string>
#include "TCPServerSpi.h"
#include "IASNetAPI.h"

TCPSeverSpi::TCPSeverSpi()
{
	m_sendCount = 0;
	m_ptrNetAPI = IASNetAPI::getInstance();
	if (m_ptrNetAPI) {
		m_ptrNetAPI->Init();
	}
}

TCPSeverSpi::~TCPSeverSpi()
{
	if (m_ptrNetAPI) {
		m_ptrNetAPI->Destroy();
	}
}

int TCPSeverSpi::Run(std::string ip, int port)
{
	m_strLocalIP = ip;
	m_nLocalPort = port;

	return Listen();
}

int TCPSeverSpi::Listen()
{
	std::string errMsg;
	m_nSocketID = m_ptrNetAPI->CreateClient(this, nullptr, m_strLocalIP, m_nLocalPort, ASNETAPI_TCP_CLIENT);
	std::cout << "fd:" << m_nSocketID << std::endl; 

	if (false == m_ptrNetAPI->Listen(m_nSocketID, 1024, errMsg)) {
		std::cout << "Start listen " << m_strLocalIP.c_str() << ":" << m_nLocalPort << " ERROR! error:" << errMsg.c_str() << std::endl;
		return -1;
	}
	std::cout << "Start listen " << m_strLocalIP.c_str() << ":" << m_nLocalPort << " OK, wait connect result!" << std::endl;

	return 0;
}
int TCPSeverSpi::Send(int m_nSocketID, std::string msg)
{
	std::string errMsg;
	if (false == m_ptrNetAPI->Send(m_nSocketID, msg.c_str(), msg.size(), errMsg)) {
		std::cout << "Call Send to " << m_strLocalIP.c_str() << ":" << m_nLocalPort << " ERROR! error:" << errMsg.c_str() << std::endl;
		return 0;
	}
	std::cout << "Call Send to " << m_strLocalIP.c_str() << ":" << m_nLocalPort << " OK, wait Send Result!" << std::endl;

	return 0;
}

int TCPSeverSpi::SendTo(int m_nSocketID, std::string msg, const std::string& strRemoteIp, unsigned short nRemotePort)
{
	std::string errMsg;
	if (false == m_ptrNetAPI->SendTo(m_nSocketID, msg.c_str(), msg.size(), strRemoteIp, nRemotePort, errMsg)) {
		std::cout << "Call Send to " << strRemoteIp.c_str() << ":" << nRemotePort << " ERROR! error:" << errMsg.c_str() << std::endl;
		return 0;
	}
	std::cout << "Call Send to " << strRemoteIp.c_str() << ":" << nRemotePort << " OK, wait Send Result!" << std::endl;

	return 0;
}

void TCPSeverSpi::OnAccept(int nListenSocketId, int nNewSocketId, const std::string& strRemoteIP, unsigned short uRemotePort)
{
	std::cout << "accept connection " << strRemoteIP.c_str() << ":" << uRemotePort << ":" << nNewSocketId <<" OK" << std::endl;
}
void TCPSeverSpi::OnConnect(int nSocketId, const std::string& strRemoteIP, unsigned short uRemotePort)
{
	;
}

void TCPSeverSpi::OnRecieve(int nSocketId, const char* pData, unsigned int nLen)
{
	std::cout << "RECV msg:" << pData << ", data len:"<< nLen << std::endl;
	std::string msg(pData, nLen);
	Send(nSocketId, msg);
}

void TCPSeverSpi::OnRecieveFrom(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIp, unsigned short nRemotePort)
{	
	std::cout << "RECV From:" << strRemoteIp.c_str() << ":" << nRemotePort <<", msg:" << pData << ", data len:" << nLen << std::endl;
	std::string msg(pData, nLen);
	SendTo(nSocketId, msg, strRemoteIp, nRemotePort);
}

void TCPSeverSpi::OnSend(int nSocketId, const char* pData, unsigned int nLen)
{
	std::cout << "send msg:" << pData << ", data len:" << nLen << " OK, wait ECHO msg!" << std::endl;
}

void TCPSeverSpi::OnClose(int nSocketId)
{
	std::string errMsg;
	std::cout << "RECV OnClose MSG, Reconnect!" << std::endl;
	if (false == m_ptrNetAPI->DestroyClient(nSocketId, errMsg)) {
		std::cout << "DestroyClient Error, msg:" << errMsg.c_str() << std::endl;
	} else {
		std::cout << "DestroyClient OK" << std::endl;
	}
}

void TCPSeverSpi::OnError(int nSocketId, const std::string& strErrorInfo)
{
	std::cout << "RECV error:" << strErrorInfo.c_str() << "call Close the connection" << std::endl;
	m_ptrNetAPI->Close(nSocketId);
}

int TCPSeverSpi::GetPacketHeaderSize()
{
	return 0;
}

int TCPSeverSpi::GetPacketTotalSize(const char* header, int length)
{
	return 0;
}

