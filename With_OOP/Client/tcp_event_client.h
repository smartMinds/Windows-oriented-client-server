#ifndef TCP_EVENT_CLIENT_H
#define TCP_EVENT_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>
// Link with Ws2_32.lib to allow using win sockets
#pragma comment (lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024

class TCPEventClient
{
public:
	TCPEventClient(const unsigned int serverPort, const std::string serverAddress);
	~TCPEventClient();

	int Init();
	int StartClient();
	int StopClient();
private:
	void ClientThread();

private:
	int serverPort;
	SOCKET clientSocket;
	std::string serverAddress;
	std::thread eventProcessThread;
	std::atomic<bool> isClientRunning;
};

#endif // TCP_EVENT_CLIENT_H
