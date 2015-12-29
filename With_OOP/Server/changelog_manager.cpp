#include "changelog_manager.h"
#include <iostream>
#include <tchar.h>  // _T()
#include <sstream>	// std::stringstream

ChangelogManager::ChangelogManager(std::vector<std::string> *changelog, 
								   std::mutex *changelogMutex, 
								   EventCallback eventCallback, 
								   const std::string dirPath) :
	changelog(changelog),
	changelogMutex(changelogMutex),
	eventCb(eventCallback),
	dirPath(dirPath)
{
}

int ChangelogManager::Init()
{
	//convert string to wstring
	std::wstring wPath = std::wstring(this->dirPath.begin(), this->dirPath.end());
	if (!CreateDirectory(wPath.c_str(), NULL))
	{		
		std::cout << "'" << this->dirPath << "'" << " - Folder Exists" << std::endl;		
	}
	else
	{
		std::cout << "'" << this->dirPath << "'" << " - Folder was created" << std::endl;
	}

	this->hDir = CreateFile(wPath.c_str(),
  							 GENERIC_READ,
							 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							 NULL,
							 OPEN_EXISTING,
							 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
							 NULL);


	return 1;
}

void ChangelogManager::StartChangelogManager()
{
	std::cout << "Changelog Manager was started" << std::endl;
	//start connections processing thread
	this->changelogManagerThread = std::thread(&ChangelogManager::ChangelogManagerThread, this);
	this->changelogManagerThread.detach();
}

void ChangelogManager::ChangelogManagerThread()
{
	OVERLAPPED o = {};
	o.hEvent = CreateEvent(0, FALSE, FALSE, 0);
	DWORD nBufferLength = N_BUFFER_LENGTH;  // events buffer length
	BYTE* lpBuffer = new BYTE[nBufferLength];
	this->isChangelogManagerRunning = true;
	this->isThreadDeinited = false;
	while (this->isChangelogManagerRunning)
	{
		DWORD returnedBytes = 0;
		ReadDirectoryChangesW(this->hDir, lpBuffer, nBufferLength, TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
			&returnedBytes, &o, 0);
		DWORD dwWaitStatus = WaitForSingleObject(o.hEvent, 100);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:
		{
			DWORD seek = 0;
			while (seek < nBufferLength)
			{
				PFILE_NOTIFY_INFORMATION pNotify = PFILE_NOTIFY_INFORMATION(lpBuffer + seek);
				seek += pNotify->NextEntryOffset;								
				std::string newEvent = this->ProcessAction(pNotify);
				
				changelogMutex->lock();
				changelog->push_back(newEvent);
				changelogMutex->unlock();
				
				//send event to all users
				eventCb(newEvent);
				
				if (pNotify->NextEntryOffset == 0)
				{
					break;
				}
			}
			break;
		}
		default:
			continue;
		}
	}
	std::cout << "Changelog Manager was stopped" << std::endl;
	CloseHandle(o.hEvent);
	delete[] lpBuffer;
	this->isThreadDeinited = true;
}

std::string ChangelogManager::ProcessAction(PFILE_NOTIFY_INFORMATION pNotify)
{
	std::wstring ws(pNotify->FileName);
	//TODO (works fine but need to pay more attention, i have no enough time to solve this problem.)
	//The reason: pNotify->FileNameLength returns filename.lenght() * 2. I guess it depends if x86 or x64 system.
	std::string fileName(ws.begin(), ws.begin() + pNotify->FileNameLength / 2);

	//parse action and return event string
	switch (pNotify->Action)
	{
		case FILE_ACTION_ADDED:
		{
			return "File \"" + fileName + "\" was added to the directory";		
		}
		case FILE_ACTION_REMOVED:
		{
			return "File \"" + fileName + "\" was removed from the directory";
		}
		case FILE_ACTION_MODIFIED:
		{
			return "File \"" + fileName + "\" was modified";		
		}
		case FILE_ACTION_RENAMED_OLD_NAME:
		{
			return "File \"" + fileName + "\" was renamed and this is the old name";
		}

		case FILE_ACTION_RENAMED_NEW_NAME:
		{
			return "File \"" + fileName + "\" was renamed and this is the new name";
		}

		default: return "";
	}
}

void ChangelogManager::StopChangelogManager()
{
	this->isChangelogManagerRunning = false;
	while (!this->isThreadDeinited) {}
}

ChangelogManager::~ChangelogManager()
{
	this->StopChangelogManager();
}
