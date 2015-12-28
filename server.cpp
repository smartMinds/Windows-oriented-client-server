// Server.cpp : Defines the entry point for the console application.
//
#include <iostream>		// cout, cerr
#include <sstream>		// std::stringstream
#include <chrono>		// std::chrono
#include <iomanip>		// to_time_t
#include <vector>		// std::vector
#include <thread>		// std::thead
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>  // _T()
#include <mutex>

#define BUFFER_SIZE 1024
#define SERVER_PORT 6666
#define QUEUE_SIZE	256
#define NEW_DIR_PATH "D:\\Buffer_folder"
#define N_BUFFER_LENGTH 10*1024
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

// Link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

void changelogManager(std::vector<SOCKET> *socketsVector, std::vector<std::string> *changelog, 
					  std::mutex *socketProcessingMutex, std::mutex *changelogMutex);



int main(int argc, char* argv[])
{
	char buffer[BUFFER_SIZE];
	std::cout << "Simple TCP server" << std::endl;
	
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

	// binding socket to local addr
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(SERVER_PORT);
	local_addr.sin_addr.s_addr = 0;
	if (bind(mysocket, (sockaddr *)&local_addr, sizeof(local_addr)))
	{
		DEBUG_PRINT("Error bind %d\n", WSAGetLastError());		
		closesocket(mysocket); // close socket
		WSACleanup(); // winsock deinit 
		return -1;
	}

	// wait for connections
	if (listen(mysocket, QUEUE_SIZE))
	{
		DEBUG_PRINT("Error listen %d\n", WSAGetLastError());		
		closesocket(mysocket); // close socket
		WSACleanup(); // winsock deinit 
		return -1;
	}

	std::cout << "Waiting for Connections" << std::endl;
	SOCKET client_socket;
	sockaddr_in client_addr;	
	int client_addr_size = sizeof(client_addr);
	std::vector<SOCKET> socketsVector;	
	std::vector<std::string> changelog;
	std::mutex socketProcessingMutex;
	std::mutex changelogMutex;

	std::thread(changelogManager, &socketsVector, &changelog, &socketProcessingMutex, &changelogMutex).detach();

	// wainting and acceptng connections
	while ((client_socket = accept(mysocket, (sockaddr *)&client_addr, &client_addr_size)))
	{
		std::stringstream sstm;
		sstm << "New connection from " << inet_ntoa(client_addr.sin_addr);		
		DEBUG_PRINT(sstm.str());
		
		//labda function to send all changelog through new connection
		std::thread addNewConnection([&client_socket, &socketsVector, &changelog, &socketProcessingMutex, &changelogMutex](){
			//send whole changelog	to new socket
			changelogMutex.lock();
			for (auto &change : changelog){
				std::string buffer(change + '\n');
				if (send(client_socket, buffer.c_str(), buffer.length(), 0) < 0)
				{
					break;
				}				
			}
			changelogMutex.unlock();
			
			socketProcessingMutex.lock();
			socketsVector.push_back(client_socket);
			socketProcessingMutex.unlock();
		});
		addNewConnection.detach();
	}

	return 0;
}






void changelogManager(std::vector<SOCKET> *socketsVector, std::vector<std::string> *changelog,
					  std::mutex *socketProcessingMutex, std::mutex *changelogMutex)
{
	if (!CreateDirectory(_T(NEW_DIR_PATH), NULL))
	{
		std::stringstream sstm;
		sstm << "'" << NEW_DIR_PATH << "'" << " - Folder Exists";
		DEBUG_PRINT(sstm.str());
	}

	HANDLE hDir = CreateFile(_T(NEW_DIR_PATH),
  							 GENERIC_READ,
							 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							 NULL,
							 OPEN_EXISTING,
							 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
							 NULL);
	OVERLAPPED o = {};
	o.hEvent = CreateEvent(0, FALSE, FALSE, 0);
	DWORD nBufferLength = N_BUFFER_LENGTH;
	BYTE* lpBuffer = new BYTE[nBufferLength];
	bool bStop = false;

	while (!bStop)
	{
		DWORD returnedBytes = 0;
		ReadDirectoryChangesW(hDir, lpBuffer, nBufferLength, TRUE,
								FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
								&returnedBytes, &o, 0);
		DWORD dwWaitStatus = WaitForSingleObject(o.hEvent, INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:
		{
			DWORD seek = 0;
			while (seek < nBufferLength)
			{
				PFILE_NOTIFY_INFORMATION pNotify = PFILE_NOTIFY_INFORMATION(lpBuffer + seek);
				seek += pNotify->NextEntryOffset;

				std::wstring ws(pNotify->FileName);
				//TODO (works fine but need to pay more attention, i have no enough time to solve this problem.)
				//The reason: pNotify->FileNameLength returns filename.lenght() * 2. I guess it depends if x86 or x64 system.
				std::string fileName(ws.begin(), ws.begin() + pNotify->FileNameLength / 2);
				std::string actionAndFileName;

				switch (pNotify->Action)
				{
				case FILE_ACTION_ADDED:
				{
					actionAndFileName = "File \"" + fileName + "\" was added to the directory";
					break;
				}
				case FILE_ACTION_REMOVED:
				{
					actionAndFileName = "File \"" + fileName + "\" was removed from the directory";
					break;
				}
				case FILE_ACTION_MODIFIED:
				{
					actionAndFileName = "File \"" + fileName + "\" was modified";
					break;
				}
				case FILE_ACTION_RENAMED_OLD_NAME:
				{
					actionAndFileName = "File \"" + fileName + "\" was renamed and this is the old name";
					break;
				}

				case FILE_ACTION_RENAMED_NEW_NAME:
				{
					actionAndFileName = "File \"" + fileName + "\" was renamed and this is the new name";
					break;
				}

				default: break;
				}
				
				changelogMutex->lock();
				changelog->push_back(actionAndFileName);
				changelogMutex->unlock();

				socketProcessingMutex->lock();
				
				for (std::vector<SOCKET>::iterator it = socketsVector->begin();	it != socketsVector->end();  )
				{
					if (send(*it, actionAndFileName.c_str(), actionAndFileName.length(), 0) < 0)
					{
						it = socketsVector->erase(it);
					} 
					else
					{
						++it;
					}						
				}
				socketProcessingMutex->unlock();

				if (pNotify->NextEntryOffset == 0)
				{
					break;
				}
			}
			break;
		}
		default:
			bStop = true;
			break;
		} // switch
	} // while
	CloseHandle(o.hEvent);
	delete[] lpBuffer;
}
