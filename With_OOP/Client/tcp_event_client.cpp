#include "tcp_event_client.h"
#include <iostream>
#include <windows.h>

TCPEventClient::TCPEventClient(const unsigned int serverPort, const std::string serverAddress) :
	serverPort(serverPort),
	serverAddress(serverAddress)
{	
	std::cout << "Simple TCP client" << std::endl;
}

TCPEventClient::~TCPEventClient()
{
  this->StopClient();
}

int TCPEventClient::Init()
{
	char buffer[BUFFER_SIZE];

	//socket library init
	WORD wVersionRequested = MAKEWORD(2, 2); // set Windows Sockets specification version
	if (WSAStartup(wVersionRequested, (WSADATA *)&buffer))
	{
		std::cout << "Error WSAStartup " << WSAGetLastError() << std::endl;
		return -1;
	}

	//create socket
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		std::cout << "Error socket " << WSAGetLastError() << std::endl;
		WSACleanup(); // winsock deinit
		return -1;
	}

	return 1;
}

int TCPEventClient::StartClient()
{
	// connection establishment
	sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(this->serverPort);
	dest_addr.sin_addr.s_addr = inet_addr((this->serverAddress).c_str());
	if (connect(clientSocket, (sockaddr *)&dest_addr, sizeof(dest_addr)))
	{
		std::cout << "Connect error " << WSAGetLastError() << std::endl;
		return -1;
	}
	std::cout << "Succesfully connected to the server" << std::endl;
	this->eventProcessThread = std::thread(&TCPEventClient::ClientThread, this);
	this->eventProcessThread.detach();
}

void TCPEventClient::ClientThread()
{
	this->isClientRunning = true;
	int nsize;
	char buffer[BUFFER_SIZE];
	while ((nsize = recv(clientSocket, &buffer[0], sizeof(buffer) - 1, 0)) != SOCKET_ERROR)
	{
		buffer[nsize] = 0;
		std::cout << buffer << std::endl;
	}
	std::cout << "Disconnected from the server" << std::endl;
	this->isClientRunning = false;
}

int TCPEventClient::StopClient()
{
	std::cout << "Shutdown client" << std::endl;
	int result = closesocket(this->clientSocket);
	if (result != 0) {
		std::cout << "Closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
	}

	while (isClientRunning) {} //wait until client didn't stopped

	return 1;
}
