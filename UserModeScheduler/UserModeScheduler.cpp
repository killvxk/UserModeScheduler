// UserModeScheduler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WebSocket.h"
#include "UserModeScheduler.h"

class TimedPrinter : public IRun {
public:
	DWORD Run() {
		for(int i = 0; i < 10; i++)
			std::cout << __FUNCTION__ << ": " << i << std::endl;
		return 0;
	}
public:
	virtual ~TimedPrinter() {
		::OutputDebugStringA(__FUNCTION__"\r\n");
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	TUmsScheduler::iUmsScheduler = new TUmsScheduler();
	TUmsScheduler::iUmsScheduler->QueueWorker(new TimedPrinter(), Normal);
	TUmsScheduler::iUmsScheduler->QueueWorker(new THttpServer("127.0.0.1", 80), Normal);
	TUmsScheduler::iUmsScheduler->Run();
	return 0;
}
