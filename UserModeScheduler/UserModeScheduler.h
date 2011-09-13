#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

namespace UmsScheduler {
	///////////////
	class IThread {
	public:
		virtual void Run() = 0;
	public:
		~IThread() {}
	};

	///////////////////////////////////////
	template <typename T> void Clear(T &t) {
		memset(&t, 0, sizeof(t));
	}

	////////////////////////////////////////////////////////////////
	inline void CheckMacro(bool check, std::string file, int line) {
		if(true == check) {
			std::stringstream dbg;
			dbg << "last_err: " << ::GetLastError() << ", file: " << file << ", line: " << line << std::endl;
			std::cout << dbg.str();
			::OutputDebugStringA(dbg.str().c_str());
		}
	}

#define Check(x) CheckMacro(x, __FILE__, __LINE__)

	//////////////////////////
	class TUmsCompletionList {
	private:
		PUMS_COMPLETION_LIST completion_list;
	public:
		TUmsCompletionList() : completion_list(NULL) {
			Check(TRUE == ::CreateUmsCompletionList(&completion_list));
		}
	public:
		PUMS_COMPLETION_LIST GetCompletionList() {
			return completion_list;
		}
	public:
		virtual ~TUmsCompletionList() {
			Check(TRUE == ::DeleteUmsCompletionList(completion_list));
			completion_list = NULL;
		}
	};
	typedef std::shared_ptr<TUmsCompletionList> TUmsCompletionListPtr;

	/////////////////////////
	class TUmsThreadContext {
	private:
		PUMS_CONTEXT ums_context;
	public:
		TUmsThreadContext() : ums_context(NULL) {
			Check(TRUE == ::CreateUmsThreadContext(&ums_context));
		}
	public:
		PUMS_CONTEXT GetTheardContext() { return ums_context; }
	private:
		BOOL SetThreadInformation(
			UMS_THREAD_INFO_CLASS threadInfoClass, PVOID threadInfo, ULONG threadInfoLength
			) {
			return ::SetUmsThreadInformation(
				ums_context, threadInfoClass, threadInfo, threadInfoLength
				);
		}
	private:
		BOOL QueryThreadInformation(
			UMS_THREAD_INFO_CLASS threadInfoClass, PVOID threadInfo, ULONG threadInfoLength, PULONG returnLength
			) {
			return ::QueryUmsThreadInformation(
				ums_context, threadInfoClass, threadInfo, threadInfoLength,	returnLength
				);
		}
	public:
		void SetThread(IThread *iThread) {
			Check(TRUE == SetThreadInformation(
				UmsThreadUserContext, &iThread,	sizeof(iThread)
				));
		}
	public:
		IThread *GetThread() {
			IThread *iThread = NULL;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				UmsThreadUserContext, &iThread, sizeof(iThread), &returnLength
				));
			Check(sizeof(iThread) == returnLength);
			return iThread;
		}
	public:
		BOOL IsSuspended() {
			BOOLEAN suspended = false;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				UmsThreadIsSuspended, &suspended, sizeof(suspended), &returnLength
				));
			Check(sizeof(suspended) == returnLength);
			return (TRUE == suspended);
		}
	public:
		BOOL IsTerminated() {
			BOOLEAN terminated = false;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				UmsThreadIsTerminated, &terminated, sizeof(terminated), &returnLength
				));
			Check(sizeof(terminated) == returnLength);
			return (TRUE == terminated);
		}
	public:
		virtual ~TUmsThreadContext() {
			Check(TRUE == ::DeleteUmsThreadContext(ums_context));
			ums_context = NULL;
		}
	};
}