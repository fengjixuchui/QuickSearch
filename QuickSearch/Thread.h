#pragma once
#include <Windows.h>  


class CThread {
	volatile bool isPause;
protected:
	HANDLE hThread;
	DWORD dwID;
	virtual void ThreadFunc() = 0;
public:
	CThread();
	~CThread();
	bool Start();
	bool Stop();
	bool Pause();
	bool IsPause();
	bool IsExist();
	static DWORD WINAPI _thread(CThread* thread);
};
