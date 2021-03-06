#ifndef __SERVERIMPL_H__
#define __SERVERIMPL_H__

#include <string>
#include "IASNetAPIClientSPI.h"
#include "IASNetAPI.h"

class TCPSeverSpi : public IASNetAPIClientSPI, IASNetAPIPacketHelper
{
public:
	TCPSeverSpi();
	~TCPSeverSpi();

	int Run(std::string ip, int port);
	int Listen();
	int Send(int nSocketId, std::string msg);
	int SendTo(int nSocketId, std::string msg, const std::string& strRemoteIp, unsigned short nRemotePort);
	int Close();

	virtual void OnAccept(int nListenSocketId, int nNewSocketId, const std::string& strRemoteIP, unsigned short uRemotePort);

	virtual void OnConnect(int nSocketId, const std::string& strRemoteIP, unsigned short uRemotePort);

	virtual void OnReceive(int nSocketId, const char* pData, unsigned int nLen);

	virtual void OnReceiveFrom(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIp, unsigned short nRemotePort);

	virtual void OnSend(int nSocketId, const char* pData, unsigned int nLen);

	virtual void OnClose(int nSocketId);

	virtual void OnError(int nSocketId, const std::string& strErrorInfo);

	virtual int GetPacketHeaderSize();

	virtual int GetPacketTotalSize(const char* header, int length);

	virtual bool PacketCheck(const char* packet, int length);
private:
	int m_nSocketID;
	int m_nLocalPort;
	int m_sendCount;
	std::string m_strLocalIP;
	
	IASNetAPI *m_ptrNetAPI;
};

#endif
