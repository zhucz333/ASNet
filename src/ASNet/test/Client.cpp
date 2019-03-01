#include <stdio.h>
#include "IASNetAPI.h"
#include "TCPClientSpi.h"
#include "TCPServerSpi.h"

int main()
{
	TCPClientSpi cltClient;
	cltClient.Run("127.0.0.1", 6666);
	getchar();	
	return 0;
}
