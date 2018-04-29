#include "Thread.h"  

CThread::CThread() {
	this->hThread = NULL;
	this->dwID = 0;
}

CThread::~CThread() {
	this->Stop();
}

bool CThread::Start() {
	bool bRet;
	if (this->isPause) {
		bRet = -1 == ::ResumeThread(this->hThread);
	}
	else {
		bRet = !(this->hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CThread::_thread, this, 0, &this->dwID));
	}
	this->isPause = false;
	return bRet;
}

bool CThread::Stop() {
	::WaitForSingleObject(this->hThread, 100);
	if (this->IsExist()) {
		::TerminateThread(this->hThread, 0);
		::CloseHandle(this->hThread);
		this->hThread = NULL;
	}
	return true;
}

bool CThread::Pause() {
	::WaitForSingleObject(this->hThread, 100);
	if (this->IsExist()) {
		::SuspendThread(this->hThread);
		this->isPause = true;
	}
	return this->isPause;
}

bool CThread::IsPause() {
	return this->IsExist() && this->isPause;
}

bool CThread::IsExist() {
	bool bRet = false;
	DWORD dwExit = 0;
	if (this->hThread && this->hThread != INVALID_HANDLE_VALUE && ::GetExitCodeThread(this->hThread, &dwExit)) {
		bRet = (STILL_ACTIVE == dwExit);
	}
	return bRet;
}

DWORD WINAPI CThread::_thread(CThread* thread) {
	thread->ThreadFunc();
	::CloseHandle(thread->hThread);
	thread->hThread = NULL;
	return 0;
}