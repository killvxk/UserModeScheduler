#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <bitset>
#include <string>
#include <sstream>
#include <memory>
#include <map>
#include <hash_map>
#include "Check.h"
#include "Interfaces.h"
#include "UserModeScheduler.h"

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

////////////////////////////////
class TSocket : public ISocket {
private:
	SOCKET socket;
public:
	TSocket() : socket(INVALID_SOCKET) {
		socket = ::WSASocketA(
			AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
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
	void Listen(int backlog) {
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
	enum { max_recv = 128 };
public:
	void Recv(std::string &data) {
		byte_array recv_buff(max_recv);
		Check(SOCKET_ERROR != ::recv(socket, &recv_buff[0], static_cast<int>(recv_buff.size()), 0));
		data.append(&recv_buff[0], recv_buff.size());
	}
public:
	void Send(const std::string &data) {
		int offset = 0;
		while(offset < data.length()) {
			int count = ::send(socket, &data.c_str()[offset], static_cast<int>(data.length()) - offset, 0);
			Check(SOCKET_ERROR != count);
			offset += count;
		}
	}
public:
	~TSocket() {
		if(INVALID_SOCKET == socket) {
			Check(SOCKET_ERROR != ::closesocket(socket));
			socket = INVALID_SOCKET;
		}
	}
};

///////////////////////////
class THttpSession : IRun {
public:
	THttpSession() { Check(false); }
private:
	IHttpSessions *iHttpSessions;
private:
	__int64 session_id;
private:
	ISocketPtr iSocket;
public:
	THttpSession(__int64 session_id, ISocketPtr iSocket, IHttpSessions *iHttpSessions) : 
	  session_id(session_id), iSocket(iSocket), iHttpSessions(iHttpSessions) {
		  TUmsScheduler::Scheduler()->QueueWorker(this, Normal);
	}
private:
	std::string recv_buff;
private:
	virtual DWORD Run() {
		for(;;) {
			iSocket->Recv(recv_buff);
		}
	}
};

////////////////////////////////////////////
class THttpSessions : public IHttpSessions {
private:
	__int64 next_session_id;
private:
	std::map<__int64 /*session_id*/, THttpSession> sessions;
private:
	IHttpServer *iHttpServer;
private:
	THttpSessions() { Check(false); }
public:
	THttpSessions(IHttpServer *iHttpServer) : 
	  iHttpServer(iHttpServer), next_session_id(0) {}
public:
	void Add(ISocketPtr iSocket) {
		sessions[next_session_id] = THttpSession(next_session_id, iSocket, this);
	}
private:
	void Remove(__int64 session_id) {
		sessions.erase(session_id);
	}
public:
	void OnData(const std::string& data) {
		//todo
		Check(false);
	}
};

/////////////////////////////////////////////////////
class THttpServer : public IHttpServer, public IRun {
private:
	TWSAStartup startUp;
private:
	THttpServer() {}
private:
	std::string nic;
private:
	short port;
private:
	ISocketPtr listener;
public:
	THttpServer(std::string nic, short port) : nic(nic), port(port) {
		listener = ISocketPtr(new TSocket());
		listener->Bind(nic, port);
		listener->Listen(SOMAXCONN);
		iHttpSessions = IHttpSessionsPtr(new THttpSessions(this));
	}
private:
	virtual void OnData(__int64 session_id, const std::string &data) {
		Check(false);
	}
private:
	IHttpSessionsPtr iHttpSessions;
private:
	DWORD Run() {
		for(;;) {
			iHttpSessions->Add(listener->Accept());
		}
		return 0;
	}
public:
	virtual ~THttpServer() {}
};
