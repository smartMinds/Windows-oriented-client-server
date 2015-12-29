#include "tcp_event_client.h"

#define SERVER_PORT 6666
#define SERVERADDR "127.0.0.1"


int main(int argc, char* argv[])
{	
	TCPEventClient * tcpEventClient = new TCPEventClient(SERVER_PORT, SERVERADDR);
	tcpEventClient->Init();
	tcpEventClient->StartClient();
	Sleep(10000);
	tcpEventClient->StopClient();
	Sleep(10000);

	return 0;
}
