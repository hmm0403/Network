#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include <windows.h>
//#include "TestMain.h"
#include "ThreadManager.h"


// 서버
// 1) 새로운 소켓 생성 (socket)
// 2) 소켓에 주소/포트 번호 설정 (bind)
// 3) 리슨 소켓 일 시키기 (listen)
// 4) 접속된 클라에 대해서 새로운 소켓을 생성 (accept)
// 5) 클라와 통신


// Select 모델 = (select 함수)
// socket set
// 1) 읽기[] 쓰기[] 예외[] 관찰 대상
// 2) select(readSet, writeSet, exceptSet); ->관찰 시작
// 3) 적어도 하나의 소켓 준비되면 리턴 -> 낙오자는 알아서 제거


// WSAEventSelect  = WSAEventSelect가 핵심이 되는
// 소켓과 관련된 네트워크 이벤트를 [이벤트 객체]를 통해 감지

// 생성 : WSACreateEvent (수동 리셋 + Manual-Reset + Non-Signaled 상태 시작)
// 삭제 : WSACloseEvent
// 신호 상태 감지 : WSAWaitForMultipleEvents
// 구체적인 네트워크 이벤트 알아내기 : WSAEnumNetworkEvents


const int32 BUF_SIZE = 1000;

// 접속한 클라이언트 하나당 클라이언트
struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;
	// 논 블로킹 방식
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	vector<WSAEVENT> wsaEvents;
	vector<Session> sessions;
	sessions.reserve(100);

	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket });

	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		// 이벤트 감지 인덱스 얻어와서
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;
		// 시작 위치 반환
		index -= WSA_WAIT_EVENT_0;

		// select 처럼
		WSANETWORKEVENTS networkEvents;
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;
		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			//Error-check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);

			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "Client Connected" << endl;
				WSAEVENT clientEvent = ::WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session{ clientSocket });
				
				// 내가 관심있는 이벤트 리드 라이트 클로스
				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}
		// Client Session 소켓 체크
		if (networkEvents.lNetworkEvents & FD_READ)
		{
			if (networkEvents.iErrorCode[FD_READ_BIT] != 0)
				continue;

			Session& s = sessions[index];

			// Read
			int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
			if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
			{
				if (recvLen <= 0)
					continue;
			}
			cout << "Recv Data = " << s.recvBuffer << endl;
			cout << "RecvLen = " << recvLen << endl;
		}

	}
	SocketUtils::Clear();
}