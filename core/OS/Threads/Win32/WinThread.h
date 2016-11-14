#ifndef _CPTHREAD_H
#define _CPTHREAD_H
#include <Windows.h>
#include <OS/Thread.h>
namespace OS {
	class CWin32Thread : public OS::CThread {
	public:
		CWin32Thread(OS::ThreadEntry *entry, void *param,  bool auto_start = true);
		~CWin32Thread();
		void start();
		void stop();
	private:
		static DWORD cwin32thread_thread(void *thread);
		DWORD m_threadid;
		HANDLE m_handle;
	};
}
#endif //_CPTHREAD_H
