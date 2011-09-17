// UserModeScheduler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UserModeScheduler.h"

class TimedPrinter : public UmsScheduler::IRun {
	DWORD Run() {
		for(int i = 0; i < 10; i++)
			std::cout << __FUNCTION__ << ": " << i << std::endl;
		return 0;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	UmsScheduler::TUmsScheduler::iUmsScheduler = new UmsScheduler::TUmsScheduler();
	UmsScheduler::TUmsScheduler::iUmsScheduler->QueueWorker(std::shared_ptr<UmsScheduler::IRun>(new TimedPrinter()), UmsScheduler::Normal);
	UmsScheduler::TUmsScheduler::iUmsScheduler->Run();
	return 0;
}
