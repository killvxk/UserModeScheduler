#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <bitset>
#include <string>
#include <sstream>
#include <memory>
#include "Check.h"

///////////////////
class TWSAStartup {
private:
	WSADATA wsaData;
public:
	TWSAStartup() {
		Clear(wsaData);
		WORD wVersionRequested = MAKEWORD(2, 2);
		Check(0 == WSAStartup(wVersionRequested, &wsaData));
		Check(LOBYTE(wsaData.wVersion) == 2 && HIBYTE(wsaData.wVersion) == 2);
	}
public:
	~TWSAStartup() {
		Check(SOCKET_ERROR != ::WSACleanup());
	}
};

/////////////////////
class IServerClient {
public:
	virtual ~IServerClient() {}
};
typedef std::shared_ptr<IServerClient> IServerClientPtr;

///////////////
class ISocket {
public:
	virtual SOCKET GetSocket() = 0;
public:
	virtual HANDLE GetHandle() = 0;
public:
	virtual void Bind(std::string nic, short port) = 0;
public:
	virtual void Connect(std::string addr, short port) = 0;
public:
	virtual ~ISocket() {}
};
typedef std::shared_ptr<ISocket> ISocketPtr;

////////////////////////////////
class TSocket : public ISocket {
private:
	SOCKET socket;
public:
	TSocket() : socket(INVALID_SOCKET) {
		socket = ::WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		Check(INVALID_SOCKET != socket);
	}
public:
	TSocket(SOCKET socket) : socket(socket) {
		Check((NULL != socket) && (INVALID_SOCKET != socket));
	}
public:
	SOCKET GetSocket() { return socket; }
public:
	HANDLE GetHandle() { return reinterpret_cast<HANDLE>(socket); }
public:
	void Bind(std::string nic, short port) {
		SOCKADDR_IN addr; Clear(addr);
		addr.sin_addr.S_un.S_addr = ::inet_addr(nic.c_str());
		addr.sin_family = AF_INET;
		addr.sin_port = ::htons(port);
		Check(SOCKET_ERROR != ::bind(socket, reinterpret_cast<LPSOCKADDR>(&addr), sizeof(addr)));
	}
public:
	void Listen(int backlog = SOMAXCONN) {
		Check(SOCKET_ERROR != ::listen(socket, backlog));
	}
public:
	ISocketPtr Accept() {
		SOCKET client = ::accept(socket, NULL, NULL);
		Check(INVALID_SOCKET != client);
		return ISocketPtr(new TSocket(client));
	}
public:
	void Connect(std::string addr, short port) {
		SOCKADDR_IN connect_addr; Clear(connect_addr);
		connect_addr.sin_addr.S_un.S_addr = ::inet_addr(addr.c_str());
		connect_addr.sin_family = AF_INET;
		connect_addr.sin_port = ::htons(port);
		Check(SOCKET_ERROR != ::connect(socket, reinterpret_cast<LPSOCKADDR>(&connect_addr), sizeof(connect_addr)));
	}
public:
	~TSocket() {
		if(INVALID_SOCKET == socket) {
			Check(SOCKET_ERROR != ::closesocket(socket));
			socket = INVALID_SOCKET;
		}
	}
};

/////////////////////////////////////////////////////////
class TServerClient : public IServerClient {
private:
	TWSAStartup startUp;
private:
	TServerClient() {}
private:
	std::string nic;
private:
	short port;
public:
	TServerClient(std::string nic, short port) : nic(nic), port(port) {

	}
private:
	DWORD Run() {
		//todo
		Check(false);
		return 0;
	}
public:
	virtual ~TServerClient() {}
};
