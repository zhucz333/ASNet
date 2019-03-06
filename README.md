## 1 ASNet库概述
<br> ASNet(asynchronous-network)库是跨平台的异步网络库，底层兼容Linux平台下的Epoll模型和Windows平台下的IOCP模型，向上提供统一的接口供调用方使用。本库的主要贡献是为网络应用的客户端和服务端提供一套接口封装简单、易用的网络库，屏蔽每个平台下网络操作的差别。本库的主要功能包括网络连接的维护、应用数据的读写、`TCP粘包`、异常事件的处理等工作，让应用专注于处理其业务逻辑。<br />
### 2 ASNet库介绍
<br>2.1 ASNet库接口类
<br>应用通过该接口来使用本库</br>
```cpp
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
	/* 异步发送操作														    */
	/* 发送数据成功，会异步回调应用层中的OnSend函数							*/
	/* 发送数据失败，会异步回调应用层中的OnError函数						*/
	/* nSocketId为CreateClient返回的连接标识符                              */
	/* pData为发送数据首地址				                                */
	/* nLen为发送数据长度													*/
	/* strRemoteIp为目的地址				                                */
	/* uRemotePort为目的端口号												*/
	/* true表示调用成功，false表示失败，errMsg失败原因						*/
	/************************************************************************/
	virtual bool SendTo(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIp, unsigned short uRemotePort, std::string &errMsg) = 0;

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
```
<br>2.2 ASNet回调函数接口</br>
<br>所有client节点必须实现该接口类</br>
```cpp
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
	virtual void OnRecieve(int nSocketId, const char* pData, unsigned int nLen) {};

	/************************************************************************/
	/* 接收网络数据回调,应用层需要处理接收到的数据包                        */
	/************************************************************************/
	virtual void OnRecieveFrom(int nSocketId, const char* pData, unsigned int nLen, const std::string& strRemoteIP, unsigned short uRemotePort) {};

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
```
<br>2.3 ASNet库TCP粘包接口</br>
<br>所有需要TCP粘包的client节点必须实现该接口类, 目前TCP粘包的接口比较单一，后续要持续增加</br>
```cpp
class IASNetAPIPacketHelper
{
public:
	IASNetAPIPacketHelper() {};
	virtual ~IASNetAPIPacketHelper() {};

	/************************************************************************/
	/* 获取PacketHeader的长度                                               */
	/************************************************************************/
	virtual int GetPacketHeaderSize() = 0;

	/************************************************************************/
	/* 根据header获取Packet的总长度，包括Header+Body+Other					*/
	/************************************************************************/
	virtual int GetPacketTotalSize(const char* header, int length) = 0;
};
```
### 3 使用方法
<br>3.1 获取、销毁ASNet实例</br>
```cpp
获取实例：
IASNetAPI& handle = IASNetAPI::getRef();
或者
IASNetAPI* ptrHandle = IASNetAPI::getInstance();

销毁实例
ptrHandle->removeInstance();
```

<br>3.2 启动、停止ASNet实例</br>
```cpp
启动实例：
ptrHandle->Init(...);;

停止实例
ptrHandle->Destroy();
```

<br>3.3 TCP server节点</br>
```cpp
创建节点：
此处需要传入回调实例和TCP粘包接口（粘包接口为可选）
int fd = ptrHandle->CreateClient(ptrCspi, packetHelper, ...);

listen：
ptrHandle->Listen(fd, ...)

accept:
当有请求到来时，会回调ptrCspi的OnAccept接口
ptrCspi->OnAccept()

send:
ptrHandle->Send(fd, ...);

发送成功：
调用ptrCspi的OnSend()接口
ptrCspi->OnSend();

recv：
当有数据到达时，会回调ptrCspi的OnRecv接口
ptrCspi->OnRecv()

error：
当有错误发生时，会回调ptrCspi的OnError接口
ptrCspi->OnError()

close：
主动关闭连接时，调用Close接口
ptrHandle->Close(fd);

被动关闭时，会回调ptrCspi的OnClose接口
ptrCspi->OnClose(fd);

清除节点：
ptrHandle->DestroyClient(fd);
```
<br>3.4 TCP client节点</br>
```cpp
创建节点：
此处需要传入回调实例和TCP粘包接口（粘包接口为可选）
int fd = ptrHandle->CreateClient(ptrCspi, packetHelper, ...);

listen：
ptrHandle->connect(fd, ...)

连接建立:
当有连接建立时，会回调ptrCspi的OnConnect接口
ptrCspi->OnConnect()

send:
ptrHandle->Send(fd, ...);

发送成功：
调用ptrCspi的OnSend()接口
ptrCspi->OnSend();

recv：
当有数据到达时，会回调ptrCspi的OnRecv接口
ptrCspi->OnRecv()

error：
当有错误发生时，会回调ptrCspi的OnError接口
ptrCspi->OnError()

close：
主动关闭连接时，调用Close接口
ptrHandle->Close(fd);

被动关闭时，会回调ptrCspi的OnClose接口
ptrCspi->OnClose(fd);

清除节点：
ptrHandle->DestroyClient(fd);
```
<br>3.5 UDP 节点</br>
```cpp
创建节点：
此处需要传入回调实例
int fd = ptrHandle->CreateClient(ptrCspi, packetHelper, ...);

send:
ptrHandle->SendTo(fd, ...);

发送成功：
调用ptrCspi的OnSend()接口
ptrCspi->OnSend();

recv：
当有数据到达时，会回调ptrCspi的OnRecv接口
ptrCspi->OnRecvFrom()

error：
当有错误发生时，会回调ptrCspi的OnError接口
ptrCspi->OnError()

清除节点：
ptrHandle->DestroyClient(fd);
```

