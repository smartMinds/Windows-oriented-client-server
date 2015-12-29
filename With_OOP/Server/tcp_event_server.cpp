#include "tcp_event_server.h"
#include <iostream>		// cout, cerr
#include <sstream>		// std::stringstream
using namespace std::placeholders; //for std::bind _1

TCPEventServer::TCPEventServer(const unsigned int serverPort, const unsigned int connectionsQueueSize) :
	serverPort(serverPort),
	connectionsQueueSize(connectionsQueueSize)
{
	std::cout << "Simple TCP server" << std::endl;
}

TCPEventServer::~TCPEventServer()
{
	delete changelogManager;
	this->StopServer();
	while (isTCPServerRunning) {}
	//deinit server
}

int TCPEventServer::Init()
{
	char buffer[BUFFER_SIZE];
	//init socket library
	WORD wVersionRequested = MAKEWORD(2, 2); // set Windows Sockets specification version
	if ( WSAStartup(wVersionRequested, (WSADATA *)&buffer) != 0 )
	{
		std::cout << "Error WSAStartup " << WSAGetLastError() << std::endl;		
		return -1;
	}

	//create socket
	if ((this->serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		std::cout << "Error socked " << WSAGetLastError() << std::endl;		
		WSACleanup(); // winsock deinit
		return -1;
	}
	
	// bind socket to local addr
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(this->serverPort);
	local_addr.sin_addr.s_addr = 0;
	if (bind(this->serverSocket, (sockaddr *)&local_addr, sizeof(local_addr)))
	{
		std::cout << "Error bind " << WSAGetLastError() << std::endl;		
		closesocket(this->serverSocket); // close socket
		WSACleanup(); // winsock deinit 
		return -1;
	}

	return 1;
}

int TCPEventServer::StartServer()
{
	//start listening from socket
	if (listen(this->serverSocket, this->connectionsQueueSize))
	{
		std::cout << "Error listen " << WSAGetLastError() << std::endl;		
		closesocket(this->serverSocket); // close socket
		WSACleanup(); // winsock deinit 
		return -1;
	}
	
	//start connections processing thread
	std::cout << "TCP Event Server was started" << std::endl;
	isTCPServerRunning = true;
	this->serverThread = std::thread(&TCPEventServer::ServerThread, this);
	this->serverThread.detach();

	//start changelog manager
	this->changelogManager = new ChangelogManager(&this->changelog, &this->changelogMutex, 
														 std::bind(&TCPEventServer::SendEventToAllUsersCallback, this, _1),
														 NEW_DIR_PATH);
	this->changelogManager->Init();
	this->changelogManager->StartChangelogManager();

	return 1;
}

int TCPEventServer::StopServer()
{
	int result = closesocket(this->serverSocket);
	if (result != 0) {
		std::cout << "Closesocket function failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();        
    }
	return result;	
}

void TCPEventServer::ServerThread()
{	
	SOCKET clientSocket;
	sockaddr_in clientAddr;
	int clientAddrSize = sizeof(clientAddr);

	// wait and accept connections
	while ( (clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrSize)) )
	{
		if (clientSocket == INVALID_SOCKET)
		{
			//error occures or server socket was closed 
			isTCPServerRunning = false;
			std::cout << "Server was stopped" << std::endl;
			return;
		}		
		std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

		if ( this->SendLogToNewUser(clientSocket) ) 
		{
			socketsProcessingMutex.lock();
			clintsSocketVector.push_back(clientSocket);
			socketsProcessingMutex.unlock();
		}
	}
}

void TCPEventServer::SendEventToAllUsersCallback(const std::string event)
{
	this->socketsProcessingMutex.lock();
	for (std::vector<SOCKET>::iterator it = this->clintsSocketVector.begin(); it != this->clintsSocketVector.end();)
	{
		if (send(*it, event.c_str(), event.length(), 0) < 0)
		{
			it = clintsSocketVector.erase(it);
		}
		else
		{
			++it;
		}
	}
	this->socketsProcessingMutex.unlock();
}

bool TCPEventServer::SendLogToNewUser(const SOCKET newClientSocket)
{
	//send whole changelog	to new socket
	changelogMutex.lock();
	bool result = true;
	//getChangelog
	for (auto &change : this->changelog){
		std::string buf(change + '\n');
		if (send(newClientSocket, buf.c_str(), buf.length(), 0) < 0)
		{
			result = false;
			break;			
		}
	}
	changelogMutex.unlock();
	return result;
}
