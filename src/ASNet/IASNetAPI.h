#ifndef __ASNETAPI_H__
#define __ASNETAPI_H__

#ifdef _WIN32
#ifdef ASNETAPI_EXPORTS
#define ASNETAPI __declspec(dllexport)
#else
#define ASNETAPI __declspec(dllimport)
#endif
#else
#define ASNETAPI
#endif

#include <string>
#include "IASNetAPIClientSPI.h"
#include "IASNetAPIPacketHelper.h"

enum ASNetAPIClientType
{
	ASNETAPI_TCP_CLIENT = 0,
	ASNETAPI_UDP_CLIENT = 1
};

class ASNETAPI IASNetAPI
{
public:
	static IASNetAPI* getInstance();
	static IASNetAPI& getRef() { return *getInstance(); };
	static void removeInstance();

public:
	/************************************************************************/
	/* 启动通讯客户端并执行监听网络操作                                     */
	/* workThreadNum: 网络收发数据的线程数									*/
	/* maxfds: 管理连接的最大数目											*/
	/* true表示启动成功，false表示失败，									*/
	/************************************************************************/
	virtual bool Init(int workThreadNum = 5, int maxfds = 1024) = 0;

	/************************************************************************/
	/* 停止通讯客户端                                                       */
	/* true表示停止成功，false表示失败，									*/
	/************************************************************************/
	virtual bool Destroy() = 0;

	/************************************************************************/
	/* 创建客户连接，但并不执行连接操作                                     */
	/* 返回连接标识ID，是每个连接的唯一标识，返回-1表示创建失败				*/
	/* 应用通过标识ID，来进行数据的收发，通信过程的控制						*/
	/************************************************************************/
	virtual int CreateClient(IASNetAPIClientSPI* cspi, IASNetAPIPacketHelper* helper = NULL, const std::string& strLocalIp = std::string(), unsigned short uLocalPort = 0, ASNetAPIClientType type = ASNETAPI_TCP_CLIENT) = 0;

	/************************************************************************/
	/* 销毁一个客户连接														*/
	/* 返回true表示成功，false表示失败										*/
	/************************************************************************/
	virtual bool DestroyClient(int nSocketId, std::string& errMsg) = 0;

	/************************************************************************/
	/* 启动服务侦听														*/
	/* 返回true表示成功，false表示失败										*/
	/************************************************************************/
	virtual bool Listen(int nSocketId, int backlog, std::string& errMsg) = 0;

	/************************************************************************/
	/* 连接建立成功，会异步回调应用层中的OnConnect函数						*/
	/* 连接建立失败，会异步回调应用层中的OnError函数						*/
	/* nSocketId为CreateClient返回的连接标识符								*/
	/* strServerIp为远程服务IP												*/
	/* uServerPort为远程服务端口											*/
	/* 返回true表示调用成功，false表示失败, errMsg失败原因					*/
	/************************************************************************/
	virtual bool Connect(int nSocketId, const std::string& strServerIp, unsigned short uServerPort, std::string &errMsg) = 0;

	/************************************************************************/
	/* 异步发送操作														    */
	/* 发送数据成功，会异步回调应用层中的OnSend函数							*/
	/* 发送数据失败，会异步回调应用层中的OnError函数						*/
	/* nSocketId为CreateClient返回的连接标识符                              */
	/* pData为发送数据首地址				                                */
	/* nLen为发送数据长度													*/
	/* true表示调用成功，false表示失败，errMsg失败原因						*/
	/************************************************************************/
	virtual bool Send(int nSocketId, const char* pData, unsigned int nLen, std::string &errMsg) = 0;

	/************************************************************************/
	/* 异步关闭连接操作												        */
	/* 关闭连接成功，会异步调用应用层中的OnClose函数						*/
	/* nSocketId为CreateClient返回的连接标识符                              */
	/* true表示调用成功，false表示失败，									*/
	/************************************************************************/
	virtual bool Close(int nSocketId = 0) = 0;

public:
	virtual ~IASNetAPI() {};
};

#endif
