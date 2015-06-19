#include <stdio.h>
#include <Windows.h>
//#include <WinSock2.h>
#include <list>
#include <errno.h>

#pragma comment (lib, "ws2_32.lib")
using namespace std;

#define PORT 7406

typedef struct _CLINET_NODE
{
	SOCKET socket;
	char userId[128];
}CLIENT_NODE, *PCLIENT_NODE;

list<CLIENT_NODE> clientList;

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	fprintf(stderr, "worker thread start...\n");

	while (1)
	{
		//fprintf(stderr, "test\n");
		char buf[256] = "";
		auto iter = clientList.begin();
		while (iter != clientList.end())
		{
			int result = recv(iter->socket, buf, 256, 0);
			if (result == 0)
			{
				// no data
				fprintf(stderr, "no data\n");
			}
			else if (result < 0)
			{
				if (WSAGetLastError() != 10035)
				{
					// error
					fprintf(stderr, "socket error - %d\n", WSAGetLastError());
					iter = clientList.erase(iter);
					continue;
				}
			}
			else if (result > 0)
			{
				fprintf(stderr, "recv(%s) : %s\n", iter->userId, buf);
				// data
			}
			iter++;
		}		
	}

	return 0;
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET svrSock;
	SOCKET clntSock;
	SOCKADDR_IN svrAddr;
	SOCKADDR_IN clntAddr;
	int addrlen = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		fprintf(stderr, "fail WSAStartup\n");
		return -1;
	}

	svrSock = socket(PF_INET, SOCK_STREAM, 0);
	if (svrSock == INVALID_SOCKET)
	{
		fprintf(stderr, "fail socket\n");
		WSACleanup();
		return -2;
	}

	memset(&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	svrAddr.sin_port = htons(PORT);

	if (bind(svrSock, (SOCKADDR*)&svrAddr, sizeof(svrAddr)) == SOCKET_ERROR)
	{
		fprintf(stderr, "fail bind\n");
		closesocket(svrSock);
		WSACleanup();
		return -3;
	}

	if (listen(svrSock, 5) == SOCKET_ERROR)
	{
		fprintf(stderr, "fail listen\n");
		closesocket(svrSock);
		WSACleanup();
		return -4;
	}

	DWORD threadId = 0;
	HANDLE hThread = NULL;
	hThread = CreateThread(NULL, 0, WorkerThread, NULL, CREATE_SUSPENDED, &threadId);
	ResumeThread(hThread);
	CloseHandle(hThread);

	while (1)
	{
		addrlen = sizeof(clntAddr);
		clntSock = accept(svrSock, (SOCKADDR*)&clntAddr, &addrlen);
		if (clntSock == INVALID_SOCKET)
		{
			fprintf(stderr, "fail accept...\n");
		}
		else
		{
			char buf[100] = "";
			recv(clntSock, buf, 100, 0);

			fprintf(stderr, "accept user : %s\n", buf);

			unsigned long arg = 1;
			ioctlsocket(clntSock, FIONBIO, &arg);

			CLIENT_NODE clntNode = { 0, };
			clntNode.socket = clntSock;
			strcpy(clntNode.userId, buf);

			clientList.push_back(clntNode);
		}
	}
	return 0;
}