#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

namespace UmsScheduler {
	///////////////
	class IUmsThread {
	public:
		virtual void Run() = 0;
	public:
		~IUmsThread() {}
	};
	typedef std::shared_ptr<IUmsThread> IUmsThreadPtr;

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
		void SetThread(IUmsThread *IUmsThread) {
			Check(TRUE == SetThreadInformation(
				UmsThreadUserContext, &IUmsThread,	sizeof(IUmsThread)
				));
		}
	public:
		IUmsThread *GetThread() {
			IUmsThread *IUmsThread = NULL;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				UmsThreadUserContext, &IUmsThread, sizeof(IUmsThread), &returnLength
				));
			Check(sizeof(IUmsThread) == returnLength);
			return IUmsThread;
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
	typedef std::shared_ptr<TUmsThreadContext> TUmsThreadContextPtr;

	//////////////////////////////////////
	class TUmsThread : public IUmsThread {
	private:
		TUmsThread() {}
	private:
		TUmsCompletionListPtr completion_list;
	private:
		HANDLE hThread;
	private:
		DWORD threadId;
	public:
		TUmsThreadContext threadContext;
	private:
		enum { KB = 1024 };
	public:
		TUmsThread(TUmsCompletionListPtr completion_list, DWORD stackSize = 1) : completion_list(completion_list), hThread(NULL), threadId(0) {
			UMS_CREATE_THREAD_ATTRIBUTES umsAttributes; Clear(umsAttributes);
			PPROC_THREAD_ATTRIBUTE_LIST pAttributeList = NULL;
			SIZE_T sizeAttributeList = 0;

			threadContext.SetThread(this);

			umsAttributes.UmsVersion = UMS_VERSION;
			umsAttributes.UmsContext = threadContext.GetTheardContext();
			umsAttributes.UmsCompletionList = completion_list->GetCompletionList();

			Check(FALSE == ::InitializeProcThreadAttributeList(NULL, 1, 0, &sizeAttributeList));
			Check(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

			std::vector<char> attributeList(sizeAttributeList);
			Check(TRUE == ::InitializeProcThreadAttributeList(pAttributeList, 1, 0, &sizeAttributeList));
			Check(TRUE == ::UpdateProcThreadAttribute(pAttributeList, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, &umsAttributes, sizeof(UMS_CREATE_THREAD_ATTRIBUTES), NULL, NULL));

			hThread = ::CreateRemoteThreadEx(
				GetCurrentProcess(), NULL, stackSize*KB, UMSThreadProxyMain, 
				this, STACK_SIZE_PARAM_IS_A_RESERVATION, pAttributeList, &hThread
				);

			::DeleteProcThreadAttributeList(pAttributeList);
		}
	};

}