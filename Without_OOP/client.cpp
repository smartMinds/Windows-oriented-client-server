// Client.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <chrono>
#include <iomanip>
#include <winsock2.h>
#include <windows.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 6666
#define SERVERADDR "127.0.0.1"
#define DEBUG_PRINT(msg) \
				                { \
                    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); \
                    struct tm result; \
                    localtime_s(&result, &now); \
                    std::cerr << "[" << std::put_time(&result, "%H:%M:%S") << "] " \
							  << msg \
                              << std::endl << std::endl \
                              << std::flush; \
				                }

using namespace std;

// Link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

int main(int argc, char* argv[])
{
	char buffer[BUFFER_SIZE];
	cout << "Simple TCP client" << endl;
	
	//socket library init
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, (WSADATA *)&buffer))
	{
		DEBUG_PRINT("Error WSAStartup %d\n", WSAGetLastError());
		return -1;
	}

	//create socket
	SOCKET mysocket;
	if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		DEBUG_PRINT("Error socket %d\n", WSAGetLastError());
		WSACleanup(); // winsock deinit
		return -1;
	}

	// connection establishment
	sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(SERVER_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(SERVERADDR);	
	
	if (connect(mysocket, (sockaddr *)&dest_addr, sizeof(dest_addr)))
	{
		DEBUG_PRINT("Connect error %d\n", WSAGetLastError());
		return -1;
	}

	DEBUG_PRINT("Succesfully connected to the server");

	int nsize;
	while ((nsize = recv(mysocket, &buffer[0], sizeof(buffer) - 1, 0)) != SOCKET_ERROR)
	{
		buffer[nsize] = 0;
		cout << buffer << endl;		
	}	

	DEBUG_PRINT("Disconnected from the server");

	return 0;
}

