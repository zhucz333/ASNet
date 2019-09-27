#ifndef __ASNETAPI_IOCPCONTEXT_H__
#define __ASNETAPI_IOCPCONTEXT_H__

#include <Winsock2.h>
#include <windows.h>

typedef enum IOCP_OPERATION_TYPE_  
{
	IOCP_CONNECT_POSTED,
	IOCP_ACCEPT_POSTED,
	IOCP_SEND_POSTED,                   
	IOCP_RECV_POSTED,
	IOCP_RECV_FROM_POSTED,
	IOCP_CLOSESOCKET_POSTED,
	IOCP_CLOSEPROCESS_POSTED,
	IOCP_NULL_POSTED
} IOCP_OPERATION_TYPE;

typedef struct ASNetAPIIOCPContext_
{
	OVERLAPPED m_Overlapped;
	WSABUF m_wsaBuffer;
	SOCKET m_nSocketId;  
	SOCKET m_nAcceptSocketId;
	struct sockaddr_in m_stRemoteAddr;
	IOCP_OPERATION_TYPE m_eOpType;
} ASNetAPIIOCPContext;

#endif