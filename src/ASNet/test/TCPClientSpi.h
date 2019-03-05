#ifndef __CLIENTIMPL_H__
#define __CLIENTIMPL_H__

#include <string>
#include "IASNetAPIClientSPI.h"
#include "IASNetAPI.h"

class TCPClientSpi : public IASNetAPIClientSPI, IASNetAPIPacketHelper
{
public:
	TCPClientSpi();
	~TCPClientSpi();

	int Run(std::string ip, int port);
	int Connect();
	int Send(int nSocketId);
	int SendTo(int nSocketId, const std::string& strRemoteIp, unsigned short nRemotePort);
	int Close();

	virtual void OnAccept(int nListenSocketId, int nNewSocketId, const std::string& strRemoteIP, unsigned short uRemotePort);

	virtual void OnConnect(int nSocketId, const std::string& strRemoteIP, unsigned short uRemotePort);

	virtual void OnRecieve(int nSocketId, const char* pData, unsigned int nLen);

	virtual void OnRecieveFrom(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIp, unsigned short nRemotePort);

	virtual void OnSend(int nSocketId, const char* pData, unsigned int nLen);

	virtual void OnClose(int nSocketId);

	virtual void OnError(int nSocketId, const std::string& strErrorInfo);

	virtual int GetPacketHeaderSize();

	virtual int GetPacketTotalSize(const char* header, int length);

private:
	int m_nSocketID;
	int m_nRemotePort;
	int m_sendCount;
	std::string m_strRemoteIP;
	
	IASNetAPI *m_ptrNetAPI;
};

#endif
