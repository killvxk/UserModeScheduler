#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <deque>

namespace UmsScheduler {
	////////////////////////////////////////
	enum EPriority { Normal = 1, High = 2 };

	//////////////////
	class IUmsThread {
	public:
		virtual DWORD Run() = 0;
	public:
		virtual void SetPriority(EPriority priority) = 0;
	public:
		virtual EPriority GetPriority() = 0;
	public:
		~IUmsThread() {}
	};
	typedef std::shared_ptr<IUmsThread> IUmsThreadPtr;

	////////////////////////////////////////
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

	////////////
	class IRun {
	public:
		virtual DWORD Run() = 0;
	public:
		virtual ~IRun() {}
	};
	typedef std::shared_ptr<IRun> IRunPtr;

	/////////////////////
	class IUmsScheduler {
	public:
		virtual PUMS_COMPLETION_LIST GetCompletionList() = 0;
	public:
		virtual void QueueWorker(IRunPtr iRun, EPriority priority) = 0;
	public:
		virtual void Dispatch() = 0;
	public:
		virtual ~IUmsScheduler() {}
	};
	typedef std::shared_ptr<IUmsScheduler> IUmsSchedulerPtr;

	//////////////////////////
	class IUmsCompletionList {
	public:
		virtual PUMS_COMPLETION_LIST GetCompletionList() = 0;
	public:
		virtual PUMS_CONTEXT GetCompletion() = 0;
	public:
		virtual HANDLE GetEvent() = 0;
	public:
		virtual ~IUmsCompletionList() {};
	};
	typedef std::shared_ptr<IUmsCompletionList> IUmsCompletionListPtr;

	/////////////////////////
	class IUmsThreadContext {
	public:
		virtual PUMS_CONTEXT GetThreadContext() = 0;
	private:
		static BOOL SetThreadInformation(
			PUMS_CONTEXT ums_context, UMS_THREAD_INFO_CLASS threadInfoClass, PVOID threadInfo, ULONG threadInfoLength
			) {
			return ::SetUmsThreadInformation(
				ums_context, threadInfoClass, threadInfo, threadInfoLength
				);
		}
	private:
		static BOOL QueryThreadInformation(
			PUMS_CONTEXT ums_context, UMS_THREAD_INFO_CLASS threadInfoClass, PVOID threadInfo, ULONG threadInfoLength, PULONG returnLength
			) {
			return ::QueryUmsThreadInformation(
				ums_context, threadInfoClass, threadInfo, threadInfoLength,	returnLength
				);
		}
	public:
		static void SetThread(PUMS_CONTEXT ums_context, IUmsThread *IUmsThread) {
			Check(TRUE == SetThreadInformation(
				ums_context, UmsThreadUserContext, &IUmsThread,	sizeof(IUmsThread)
				));
		}
	public:
		static IUmsThread *GetThread(PUMS_CONTEXT ums_context) {
			IUmsThread *IUmsThread = NULL;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				ums_context, UmsThreadUserContext, &IUmsThread, sizeof(IUmsThread), &returnLength
				));
			Check(sizeof(IUmsThread) == returnLength);
			return IUmsThread;
		}
	public:
		static BOOL IsSuspended(PUMS_CONTEXT ums_context) {
			BOOLEAN suspended = false;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				ums_context, UmsThreadIsSuspended, &suspended, sizeof(suspended), &returnLength
				));
			Check(sizeof(suspended) == returnLength);
			return (TRUE == suspended);
		}
	public:
		static BOOL IsTerminated(PUMS_CONTEXT ums_context) {
			BOOLEAN terminated = false;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				ums_context, UmsThreadIsTerminated, &terminated, sizeof(terminated), &returnLength
				));
			Check(sizeof(terminated) == returnLength);
			return (TRUE == terminated);
		}
	public:
		virtual ~IUmsThreadContext() {}
	};
	typedef std::shared_ptr<IUmsThreadContext> IUmsThreadContextPtr;

	//////////////////////////////////////////////////////
	class TUmsCompletionList : public IUmsCompletionList {
	private:
		PUMS_COMPLETION_LIST completion_list;
	private:
		HANDLE hEvent;
	public:
		TUmsCompletionList() : completion_list(NULL), hEvent(NULL) {
			Check(TRUE == ::CreateUmsCompletionList(&completion_list));
			Check(TRUE == ::GetUmsCompletionListEvent(completion_list, &hEvent));
		}
	public:
		HANDLE GetEvent() { return hEvent; }
	public:
		PUMS_COMPLETION_LIST GetCompletionList() {
			return completion_list;
		}
	private:
		std::deque<PUMS_CONTEXT> completions;
	public:
		PUMS_CONTEXT GetCompletion() {
			DequeueCompletions();
			while(completions.size() > 0) {
				PUMS_CONTEXT front = completions.front();
				completions.pop_front();
				if(TRUE == IUmsThreadContext::IsTerminated(front)) {
					delete IUmsThreadContext::GetThread(front);
					continue;
				} else if(TRUE == IUmsThreadContext::IsSuspended(front)) {
					completions.push_back(front);
					continue;
				} else {
					return front;
				}
			}
			return NULL;
		}
	private:
		void DequeueCompletions() {
			PUMS_CONTEXT completion = NULL;
			Check(TRUE == ::DequeueUmsCompletionListItems(completion_list, 0, &completion));
			while(NULL != completion) {
				IUmsThread *thread = IUmsThreadContext::GetThread(completion);
				if((NULL != thread) && (thread->GetPriority() == High))
					completions.push_front(completion);
				else
					completions.push_back(completion);
				completion = ::GetNextUmsListItem(completion);
			}
		}
	public:
		virtual ~TUmsCompletionList() {
			Check(TRUE == ::DeleteUmsCompletionList(completion_list));
			completion_list = NULL;
			Check(TRUE == ::CloseHandle(hEvent));
			hEvent = NULL;
		}
	};

	////////////////////////////////////////////////////
	class TUmsThreadContext : public IUmsThreadContext {
	private:
		PUMS_CONTEXT ums_context;
	public:
		TUmsThreadContext() : ums_context(NULL) {
			Check(TRUE == ::CreateUmsThreadContext(&ums_context));
		}
	public:
		PUMS_CONTEXT GetThreadContext() { return ums_context; }
	public:
		virtual ~TUmsThreadContext() {
			Check(TRUE == ::DeleteUmsThreadContext(ums_context));
			ums_context = NULL;
		}
	};

	//////////////////////////////////////
	class TUmsThread : public IUmsThread {
	private:
		TUmsThread() {}
	private:
		IUmsCompletionListPtr completion_list;
	private:
		HANDLE hThread;
	private:
		DWORD threadId;
	public:
		TUmsThreadContext threadContext;
	private:
		IRunPtr iRun;
	private:
		enum { KB = 1024 };
	private:
		EPriority priority;
	public:
		void SetPriority(EPriority priority) { this->priority = priority; }
	public:
		EPriority GetPriority() { return priority; }
	public:
		DWORD Run() { return iRun->Run(); }
	private:
		static DWORD CALLBACK UMSThreadProxyMain(LPVOID lpParameter)
		{
			TUmsThread *thread = reinterpret_cast<TUmsThread*>(lpParameter);
			return thread->Run();
		}
	public:
		TUmsThread(IUmsCompletionListPtr completion_list, IRunPtr iRun, EPriority priority = Normal, DWORD stackSize = 1) : 
		  completion_list(completion_list), hThread(NULL), threadId(0), iRun(iRun), priority(priority) {

			UMS_CREATE_THREAD_ATTRIBUTES umsAttributes; Clear(umsAttributes);
			PPROC_THREAD_ATTRIBUTE_LIST pAttributeList = NULL;
			SIZE_T sizeAttributeList = 0;

			threadContext.SetThread(threadContext.GetThreadContext(), this);
			umsAttributes.UmsVersion = UMS_VERSION;
			umsAttributes.UmsContext = threadContext.GetThreadContext();
			umsAttributes.UmsCompletionList = completion_list->GetCompletionList();

			Check(FALSE == ::InitializeProcThreadAttributeList(NULL, 1, 0, &sizeAttributeList));
			Check(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

			std::vector<char> attributeList(sizeAttributeList);
			pAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(&attributeList[0]);
			Check(TRUE == ::InitializeProcThreadAttributeList(pAttributeList, 1, 0, &sizeAttributeList));
			Check(TRUE == ::UpdateProcThreadAttribute(pAttributeList, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, &umsAttributes, sizeof(UMS_CREATE_THREAD_ATTRIBUTES), NULL, NULL));

			hThread = ::CreateRemoteThreadEx(
				GetCurrentProcess(), NULL, stackSize*KB, UMSThreadProxyMain, 
				this, STACK_SIZE_PARAM_IS_A_RESERVATION, pAttributeList, &threadId
				);
			Check(NULL != hThread);
			::DeleteProcThreadAttributeList(pAttributeList);
		}
	};

	////////////////////////////////////////////
	class TUmsScheduler : public IUmsScheduler {
	private:
		IUmsCompletionListPtr completion_list;
	public:
		virtual PUMS_COMPLETION_LIST GetCompletionList() { return completion_list->GetCompletionList(); }
	private:
		__int64 threadCount;
	private:
		static __declspec(thread) IUmsScheduler *iUmsScheduler;
	public:
		TUmsScheduler() : threadCount(0) {
			completion_list = IUmsCompletionListPtr(new TUmsCompletionList());
			Check(NULL == iUmsScheduler);
			iUmsScheduler = new TUmsScheduler();
		}
	public:
		virtual void Dispatch() {
			PUMS_CONTEXT context = completion_list->GetCompletion();
			if(NULL != context) {
				Check(TRUE == ::ExecuteUmsThread(context));
			}
		}
	public:
		virtual void QueueWorker(IRunPtr iRun, EPriority priority) {
			threadCount++;
			new TUmsThread(completion_list, iRun, priority);
		}
	public:
		static IUmsScheduler *Scheduler() {
			Check(false);
			return NULL;
		}
	private:
		static VOID NTAPI SchedulerProc(
			RTL_UMS_SCHEDULER_REASON Reason, ULONG_PTR ActivationPayload, PVOID SchedulerParam
		) {
			Scheduler()->Dispatch();
		}
	public:
		void Run() {
			UMS_SCHEDULER_STARTUP_INFO startupInfo; Clear(startupInfo);
			startupInfo.CompletionList = iUmsScheduler->GetCompletionList();
			startupInfo.SchedulerParam = NULL;
			startupInfo.SchedulerProc = SchedulerProc;
			startupInfo.UmsVersion = UMS_VERSION;
			Check(TRUE == ::EnterUmsSchedulingMode(&startupInfo));
		}
	public:
		virtual ~TUmsScheduler() {
			Check(NULL != iUmsScheduler);
			delete iUmsScheduler;
			iUmsScheduler = NULL;
		}
	};
	__declspec(thread) IUmsScheduler *TUmsScheduler::iUmsScheduler = NULL;

}