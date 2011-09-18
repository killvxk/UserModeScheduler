#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <deque>
#include "Check.h"
#include "Interfaces.h"

namespace UmsScheduler {
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
		static void SetThread(PUMS_CONTEXT ums_context, IUmsThread *iUmsThread) {
			Check(TRUE == SetThreadInformation(
				ums_context, UmsThreadUserContext, &iUmsThread,	sizeof(IUmsThread)
				));
		}
	public:
		static IUmsThread *GetThread(PUMS_CONTEXT ums_context) {
			IUmsThread *iUmsThread = NULL;
			ULONG returnLength = 0;
			Check(TRUE == QueryThreadInformation(
				ums_context, UmsThreadUserContext, &iUmsThread, sizeof(iUmsThread), &returnLength
				));
			Check(sizeof(IUmsThread) == returnLength);
			return iUmsThread;
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
		class TUmsThreadCount : public IUmsThreadCount {
		public:
			TUmsThreadCount() : count(0) {}
		private:
			int count;
		public:
			void Increment() { count++; }
		public:
			void Decrement() { count--; }
		public:
			bool ReadyToExit() { Check(count >= 0); return (count == 0); }
		} threadCount;
	private:
		IUmsThreadCount *ThreadCount() { return &threadCount; }
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
					threadCount.Decrement();
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
				Check(NULL != thread);
				if(thread->GetPriority() == High)
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
		IUmsScheduler *scheduler;
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
	public:
		IUmsScheduler *GetScheduler() { return scheduler; }
	private:
		static DWORD CALLBACK UMSThreadProxyMain(LPVOID lpParameter)
		{
			TUmsThread *thread = reinterpret_cast<TUmsThread*>(lpParameter);
			return thread->Run();
		}
	public:
		TUmsThread(IUmsScheduler *scheduler, IUmsCompletionListPtr completion_list, 
			IRunPtr iRun, EPriority priority = Normal, DWORD stackSize = 1) : 
		  scheduler(scheduler), completion_list(completion_list), hThread(NULL), 
			  threadId(0), iRun(iRun), priority(priority) {

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
			Check(TRUE == ::UpdateProcThreadAttribute(
				pAttributeList, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, 
				&umsAttributes, sizeof(UMS_CREATE_THREAD_ATTRIBUTES), NULL, NULL));

			hThread = ::CreateRemoteThreadEx(
				GetCurrentProcess(), NULL, stackSize*KB, UMSThreadProxyMain, 
				this, STACK_SIZE_PARAM_IS_A_RESERVATION, pAttributeList, &threadId
				);
			Check(NULL != hThread);
			::DeleteProcThreadAttributeList(pAttributeList);
		}
	public:
		virtual ~TUmsThread() {	}
	};

	////////////////////////////////////////////
	class TUmsScheduler : public IUmsScheduler {
	private:
		IUmsCompletionListPtr completion_list;
	public:
		virtual PUMS_COMPLETION_LIST GetCompletionList() { return completion_list->GetCompletionList(); }
	public:
		static __declspec(thread) IUmsScheduler *iUmsScheduler;
	public:
		TUmsScheduler() {
			completion_list = IUmsCompletionListPtr(new TUmsCompletionList());
			Check(NULL == iUmsScheduler);
		}
	public:
		virtual void Dispatch() {
			for(;;) {
				PUMS_CONTEXT context = completion_list->GetCompletion();
				if(NULL != context) {
					Check(TRUE == ::ExecuteUmsThread(context));
					break;
				} else {
					if(false == completion_list->ThreadCount()->ReadyToExit())
						Check(WAIT_OBJECT_0 == ::WaitForSingleObject(completion_list->GetEvent(), INFINITE));
					else
						break;
				}
			}
		}
	public:
		virtual void QueueWorker(IRunPtr iRun, EPriority priority) {
			completion_list->ThreadCount()->Increment();
			new TUmsThread(this, completion_list, iRun, priority);
		}
	public:
		static IUmsScheduler *Scheduler() {
			PUMS_CONTEXT umsContext = ::GetCurrentUmsThread();
			if(NULL == umsContext) { // non-UMS thread
				Check(NULL != TUmsScheduler::iUmsScheduler);
				return TUmsScheduler::iUmsScheduler;
			} else {
				IUmsThread *thread = IUmsThreadContext::GetThread(umsContext);
				if(NULL == thread) { // UMS mode, scheduler
					Check(NULL != TUmsScheduler::iUmsScheduler);
					return TUmsScheduler::iUmsScheduler;
				} else { // UMS mode, worker
					Check(NULL != thread->GetScheduler());
					return thread->GetScheduler();
				}
			}
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
		}
	};
	__declspec(thread) IUmsScheduler *TUmsScheduler::iUmsScheduler = NULL;

}