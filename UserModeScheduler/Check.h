#pragma once
#include <Windows.h>
#include <string>
#include <sstream>
#include <iostream>

////////////////////////////////////////////////////////////////
inline void CheckMacro(bool check, std::string file, int line) {
	if(!check) {
		std::stringstream dbg;
		dbg << "last_err: " << ::GetLastError() << ", file: " << file << ", line: " << line << std::endl;
		std::cout << dbg.str();
		::OutputDebugStringA(dbg.str().c_str());
		DebugBreak();
	}
}

//////////////////////////////////////////////////
#define Check(x) CheckMacro(x, __FILE__, __LINE__)

////////////////////////////////////////
template <typename T> void Clear(T &t) {
	memset(&t, 0, sizeof(t));
}
