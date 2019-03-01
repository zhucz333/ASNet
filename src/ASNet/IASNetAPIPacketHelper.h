#ifndef __ASNETAPI_PACKETHELPER_H__
#define __ASNETAPI_PACKETHELPER_H__

#include <string>

/************************************************************************/
/* IASNetAPIPacketHelper是应用层提供的包解析器，应用如果在使用本库时，  */
/* 如果需要Net库返回一个完整的Packet，则需要实现该接口，并在创建连接时，*/
/* 将该接口的实现注册到连接中											*/
/* 使用的具体步骤如下：													*/
/* 1）Net库通过GetPacketHeaderSize()获得Header的长度HeaderSize			*/
/* 2）Net库接收到Header后，调用GetPacketTotalSize()获得整个包长度		*/
/* 3）Net库接收完整的包后，通过IHTClientLisenter.OnReceive返回给应用	*/
/************************************************************************/
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

#endif // !__NET_CLIENTPACKETHELPER_H__
