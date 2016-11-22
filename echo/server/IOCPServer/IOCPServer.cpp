// IOCPServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <cstdio>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024
#define SERVER_PORT 3500

struct SOCKETINFO {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	SOCKET socket;
	char messageBuffer[MAX_BUFFER];
	int receiveBytes;
	int sendBytes;
};

DWORD WINAPI makeThread(LPVOID hIOCP);

int main(int argc, char* argv[])
{
	WSAData WSAData;
	if (::WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		_STD printf("Error - Can not load 'winsock.dll' file\n");
		return 1;
	}

	SOCKET listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		_STD printf("Error - Invalid socket\n");
		return 1;
	}

	SOCKADDR_IN serverAddr;
	::ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (::bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		_STD printf("Error - Fail bind\n");
		::closesocket(listenSocket);
		::WSACleanup();
		return 1;
	}

	if (::listen(listenSocket, 5) == SOCKET_ERROR) {
		_STD printf("Error - Fail listen\n");
		::closesocket(listenSocket);
		::WSACleanup();
		return 1;
	}

	HANDLE hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	int threadCount = systemInfo.dwNumberOfProcessors * 2;
	unsigned long threadId;
	HANDLE *hThread = (HANDLE *)malloc(threadCount * sizeof(HANDLE));
	for (int i = 0; i < threadCount; i++) {
		hThread[i] = ::CreateThread(NULL, 0, makeThread, &hIOCP, 0, &threadId);
	}

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);
	::ZeroMemory(&clientAddr, addrLen);
	SOCKET clientSocket;
	SOCKETINFO *socketInfo;
	DWORD receiveBytes;
	DWORD flags;

	for (; 1; ) {
		clientSocket = ::accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {
			_STD printf("Error - Accept Failure\n");
			return 1;
		}

		socketInfo = (struct SOCKETINFO *)malloc(sizeof(*socketInfo));
		::ZeroMemory(socketInfo, sizeof(*socketInfo));
		socketInfo->socket = clientSocket;
		socketInfo->receiveBytes = 0;
		socketInfo->sendBytes = 0;
		socketInfo->dataBuffer.len = MAX_BUFFER;
		socketInfo->dataBuffer.buf = socketInfo->messageBuffer;
		flags = 0;

		hIOCP = ::CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (DWORD)socketInfo, 0);

		if (::WSARecv(socketInfo->socket, &socketInfo->dataBuffer, 1, &receiveBytes, &flags, &(socketInfo->overlapped), NULL)) {
			if (::WSAGetLastError() != WSA_IO_PENDING) {
				_STD printf("Error - IO pending Failuere\n");
				return 1;
			}
		}
	}

	::closesocket(listenSocket);
	::WSACleanup();

	return 0;
}

DWORD WINAPI makeThread(LPVOID hIOCP)
{
	HANDLE threadHandler = *((HANDLE *)hIOCP);
	DWORD receiveBytes;
	DWORD sendBytes;
	DWORD completionKey;
	DWORD flags;
	struct SOCKETINFO *eventSocket;

	for (; 1; ) {
		if (::GetQueuedCompletionStatus(threadHandler, &receiveBytes, &completionKey, (LPOVERLAPPED *)&eventSocket, INFINITE) == 0) {
			_STD printf("Error - GetQueuedCompletionStatus Failure\n");
			::closesocket(eventSocket->socket);
			::free(eventSocket);
			return 1;
		}

		eventSocket->dataBuffer.len = receiveBytes;

		if (receiveBytes > 0) {
			_STD printf("TRACE - Receive message : %s (%d bytes)\n", eventSocket->dataBuffer.buf, eventSocket->dataBuffer.len);

			if (::WSASend(eventSocket->socket, &(eventSocket->dataBuffer), 1, &sendBytes, 0, NULL, NULL) == SOCKET_ERROR) {
				if (::WSAGetLastError() != WSA_IO_PENDING) {
					_STD printf("Error - Fail WSASend(error_Code : %d)\n", ::WSAGetLastError());
				}
			}

			_STD printf("TRACE - Send message : %s (%d bytes)", eventSocket->dataBuffer.buf, eventSocket->dataBuffer.len);

			::ZeroMemory(eventSocket->messageBuffer, MAX_BUFFER);
			eventSocket->receiveBytes = 0;
			eventSocket->sendBytes = 0;
			eventSocket->dataBuffer.len = MAX_BUFFER;
			eventSocket->dataBuffer.buf = eventSocket->messageBuffer;
			flags = 0;

			if (::WSARecv(eventSocket->socket, &(eventSocket->dataBuffer), 1, &receiveBytes, &flags, &eventSocket->overlapped, NULL) == SOCKET_ERROR) {
				if (::WSAGetLastError() != WSA_IO_PENDING) {
					_STD printf("Error - Fail WSARecv(error_code : %d)\n", ::WSAGetLastError());
				}
			}
		} else {
			::closesocket(eventSocket->socket);
			free(eventSocket);
		}
	}
	return 0;
}
