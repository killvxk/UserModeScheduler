// UserModeScheduler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UserModeScheduler.h"

int _tmain(int argc, _TCHAR* argv[])
{
	UmsScheduler::TUmsScheduler scheduler;
	scheduler.Run();
	return 0;
}

