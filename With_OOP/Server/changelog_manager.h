#ifndef CHANGELOG_MANAGER_H
#define CHANGELOG_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <windows.h>

#define N_BUFFER_LENGTH 10*1024

typedef std::function<void(std::string event)> EventCallback;

class ChangelogManager
{
public:
	ChangelogManager(std::vector<std::string> *changelog, std::mutex *changelogMutex, EventCallback eventCallback, const std::string dirPath);
	int Init();
	~ChangelogManager();
	void StartChangelogManager();
	void StopChangelogManager();
private: 
	void ChangelogManagerThread();
	std::string ProcessAction(PFILE_NOTIFY_INFORMATION pNotify);

private:
	std::vector<std::string> *changelog;
	std::mutex *changelogMutex;
	EventCallback eventCb;
	std::thread changelogManagerThread;
	std::string dirPath;
	std::atomic<bool> isChangelogManagerRunning;
	std::atomic<bool> isThreadDeinited; //to wait until thread will deinit locals
	HANDLE hDir;
};
#endif
