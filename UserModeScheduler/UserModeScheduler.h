#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

namespace UmsScheduler {
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


	class TUmsThreadContext {
	private:
		PUMS_CONTEXT ums_context;
	public:
		TUmsThreadContext() : ums_context(NULL) {
			Check(TRUE == ::CreateUmsThreadContext(&ums_context));
		}
	public:
		PUMS_CONTEXT GetTheardContext() { return ums_context; }
	public:
		virtual ~TUmsThreadContext() {
			Check(TRUE == ::DeleteUmsThreadContext(ums_context));
			ums_context = NULL;
		}
	};
}