#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <string>
#include <memory>
#include <vector>

/////////////////////////////////////
typedef std::vector<char> byte_array;

///////////////////
class IHttpServer {
public:
	virtual void OnData(__int64 session_id, const std::string &data) = 0;
public:
	virtual ~IHttpServer() {}
};
typedef std::shared_ptr<IHttpServer> IHttpServerPtr;

///////////////
class ISocket {
public:
	virtual SOCKET GetSocket() = 0;
public:
	virtual HANDLE GetHandle() = 0;
public:
	virtual void Bind(std::string nic, short port) = 0;
public:
	virtual void Listen(int backlog) = 0;
public:
	virtual std::shared_ptr<ISocket> Accept() = 0;
public:
	virtual void Connect(std::string addr, short port) = 0;
public:
	virtual void Recv(std::string &data) = 0;
public:
	virtual void Send(const std::string &data) = 0;
public:
	virtual ~ISocket() {}
};
typedef std::shared_ptr<ISocket> ISocketPtr;

/////////////////
class ISessions {
public:
	virtual void Add(ISocketPtr iSocket) = 0;
public:
	virtual void Remove(__int64 session_id) = 0;
public:
	virtual void OnData(const std::string& data) = 0;
};
typedef std::shared_ptr<ISessions> ISessionsPtr;

////////////////////
class IUmsScheduler;

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
	virtual IUmsScheduler *GetScheduler() = 0;
public:
	virtual ~IUmsThread() { }
};
typedef std::shared_ptr<IUmsThread> IUmsThreadPtr;

////////////
class IRun {
public:
	virtual DWORD Run() = 0;
public:
	virtual ~IRun() {}
};
typedef std::shared_ptr<IRun> IRunPtr;

///////////////////////
class IUmsThreadCount {
	public:
		virtual void Increment() = 0;
	public:
		virtual bool ReadyToExit() = 0;
};

/////////////////////
class IUmsScheduler {
public:
	virtual PUMS_COMPLETION_LIST GetCompletionList() = 0;
public:
	virtual void QueueWorker(IRun *iRun, EPriority priority) = 0;
public:
	virtual void Dispatch() = 0;
public:
	virtual void Run() = 0;
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
	virtual IUmsThreadCount *ThreadCount() = 0;
public:
	virtual ~IUmsCompletionList() {};
};
typedef std::shared_ptr<IUmsCompletionList> IUmsCompletionListPtr;
