#ifndef TCP_EVENT_SERVER_H
#define TCP_EVENT_SERVER_H

#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include "changelog_manager.h"

#define BUFFER_SIZE 1024
#define NEW_DIR_PATH "D:\\Buffer_folder"

// Link with Ws2_32.lib to allow using win sockets
#pragma comment (lib, "Ws2_32.lib")

class TCPEventServer
{
public:
	TCPEventServer(const unsigned int serverPort, const unsigned int connectionsQueueSize);
	~TCPEventServer();
	int Init();
	int StartServer();
	int StopServer();
	void SendEventToAllUsersCallback(const std::string event);
	
	//if method returns true - need to add socket to clientsSocketList, else - don't add.
	bool SendLogToNewUser(const SOCKET newClientSocket);

private:
	int createServerSocket();
	void ServerThread();

private:
	unsigned int serverPort;
	unsigned int connectionsQueueSize;	
	std::thread serverThread;
	
	SOCKET serverSocket;
	std::vector<SOCKET> clintsSocketVector;
	std::mutex socketsProcessingMutex;

	std::vector<std::string> changelog;
	std::mutex changelogMutex;
	std::atomic<bool> isTCPServerRunning;
	
	ChangelogManager * changelogManager;
};

#endif // TCP_SERVER_H
