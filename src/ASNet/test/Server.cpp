#include <stdio.h>
#include "IASNetAPI.h"
#include "TCPClientSpi.h"
#include "TCPServerSpi.h"

int main()
{
	TCPSeverSpi svrClient;
	svrClient.Run("127.0.0.1", 6666);

	getchar();

	svrClient.Close();

	getchar();

	return 0;
}
