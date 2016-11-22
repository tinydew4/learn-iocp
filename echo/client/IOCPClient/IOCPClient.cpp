// IOCPClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <cstdio>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024
#define SERVER_IP _T("127.0.0.1")
#define SERVER_PORT 3500

struct SOCKETINFO {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	int receiveBytes;
	int sendBytes;
};

int main()
{
	WSADATA WSAData;
	if (::WSAStartup(MAKEWORD(2, 0), &WSAData) != 0) {
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
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	InetPton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

	if (::connect(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		_STD printf("Error - Fail to connect\n");
		::closesocket(listenSocket);
		::WSACleanup();
		return 1;
	} else {
		_STD printf("Server Connected\n* Enter Message\n->");
	}

	SOCKETINFO *socketInfo;
	DWORD sendBytes;
	DWORD receiveBytes;
	DWORD flags;

	for (; 1; ) {
		char messageBuffer[MAX_BUFFER];
		int i, bufferLen;
		for (i = 0; 1; i++) {
			messageBuffer[i] = getchar();
			if (messageBuffer[i] == '\n') {
				messageBuffer[i] = '\0';
				break;
			}
		}
		bufferLen = i;

		socketInfo = (struct SOCKETINFO *)malloc(sizeof(*socketInfo));
		::ZeroMemory(socketInfo, sizeof(*socketInfo));
		socketInfo->dataBuffer.len = bufferLen;
		socketInfo->dataBuffer.buf = messageBuffer;

		int sendBytes = ::send(listenSocket, messageBuffer, bufferLen, 0);
		if (sendBytes > 0) {
			_STD printf("TRACE - Send message : %s (%d bytes)\n", messageBuffer, sendBytes);

			int receiveBytes = recv(listenSocket, messageBuffer, MAX_BUFFER, 0);
			if (receiveBytes > 0) {
				_STD printf("TRACE - Receive message : %s (%d bytes)\n* Enter Message\n->", messageBuffer, receiveBytes);
			}
		}
	}

	::closesocket(listenSocket);
	::WSACleanup();

    return 0;
}

