#ifndef __ASNETAPI_CLIENTSPI_H__
#define __ASNETAPI_CLIENTSPI_H__

#include <string>

/************************************************************************/
/* IASNetAPIClientSPI是应用层中客户端的通信基类							*/
/* 必须要实现该接口，才可以作为客户端进行通信							*/
/************************************************************************/
class  IASNetAPIClientSPI
{
public:
	IASNetAPIClientSPI() {};
	virtual ~IASNetAPIClientSPI() {};

	/************************************************************************/
	/* 连接成功回调                                                         */
	/************************************************************************/
	virtual void OnAccept(int nListenSocketId, int nNewSocketId, const std::string& strRemoteIP, unsigned short uRemotePort) {};

	/************************************************************************/
	/* 连接成功回调                                                         */
	/************************************************************************/
	virtual void OnConnect(int nSocketId, const std::string& strRemoteIP, unsigned short uRemotePort) {};

	/************************************************************************/
	/* 接收网络数据回调,应用层需要处理接收到的数据包                        */
	/************************************************************************/
	virtual void OnReceive(int nSocketId, const char* pData, unsigned int nLen) {};

	/************************************************************************/
	/* 接收网络数据回调,应用层需要处理接收到的数据包                        */
	/************************************************************************/
	virtual void OnReceiveFrom(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIP, unsigned short uRemotePort) {};

	/************************************************************************/
	/* 发送数据成功回调                                                     */
	/************************************************************************/
	virtual void OnSend(int nSocketId, const char* pData, unsigned int nLen) = 0;

	/************************************************************************/
	/* 已关闭套接字回调                                                     */
	/************************************************************************/
	virtual void OnClose(int nSocketId) = 0;

	/************************************************************************/
	/* 错误回调                                                             */
	/************************************************************************/
	virtual void OnError(int nSocketId, const std::string& strErrorInfo) = 0;
};

#endif // !__ASNETAPI_CLIENTSPI_H__
