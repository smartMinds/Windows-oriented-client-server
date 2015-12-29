#include "tcp_event_server.h"

#define SERVER_PORT 6666
#define CONNECTONS_QUEUE_SIZE 256

int main(int argc, char* argv[])
{
	TCPEventServer * tcpEventServer = new TCPEventServer(SERVER_PORT, CONNECTONS_QUEUE_SIZE);
	tcpEventServer->Init();
	tcpEventServer->StartServer();
	//time when server is working. Then deinit server.
	Sleep(1000000);
	delete tcpEventServer;	
	Sleep(10000);

	return 0;
}
