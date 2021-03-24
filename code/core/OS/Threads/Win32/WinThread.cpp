#include <OS/OpenSpy.h>
#include <OS/Task.h>
#include <OS/Thread.h>
#include "WinThread.h"

#include <Windows.h>
namespace OS {
	DWORD CWin32Thread::cwin32thread_thread(void *thread) {
		CWin32Thread *t = (CWin32Thread *)thread;
		t->m_entry(t);
		//OS::Sleep(TASK_SLEEP_TIME);
		//delete t;
		return 0;
	}
	CWin32Thread::CWin32Thread(OS::ThreadEntry *entry, void *param, bool auto_start) : OS::CThread(entry, param, auto_start) {
		m_running = false;
		m_params = param;
		if(auto_start) {
			start();
		}
		m_entry = entry;

	}
	CWin32Thread::~CWin32Thread() {
		TerminateThread(m_handle, 0);
	}
	void CWin32Thread::start() {
		if(!m_running) {
			m_running = true;
			m_handle = ::CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)cwin32thread_thread, this, 0, &m_threadid);			
		}
	}
	void CWin32Thread::stop() {
		if(m_running) {
			TerminateThread(m_handle, 0);
			m_running = false;
		}
	}
	void CWin32Thread::SignalExit(bool wait, CThreadPoller* poller) {
		m_running = false;
		if (poller) {
			poller->signal();
		}
		if(wait)
			WaitForSingleObject(m_handle, INFINITE);
	}
}
