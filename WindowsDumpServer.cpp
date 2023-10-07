#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define success(str) std::cout << "[+] " << str << std::endl
#define error(str) std::cout << "[-] " << str << std::endl
#define info(str) std::cout << "[*] " << str << std::endl

int main()
{
	const char* ip = "0.0.0.0";
	int port = 4000;

	//Creating TCP server
	WSAData data;
	WSAStartup(MAKEWORD(2, 2), &data);

	info("Creating socket");
	SOCKET serverSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, NULL);

	if (serverSock == SOCKET_ERROR)
	{
		error("Error creating socket " + std::to_string(GetLastError()));
		return 0;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET; //Socket family (IPV4, IPV6, etc)
	addr.sin_port = htons(port); //Setting the port number in network format
	InetPtonA(AF_INET, ip, &addr.sin_addr); //Setting the ip address to network format

	info("Binding port");
	if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
	{
		error("Bind error " + std::to_string(GetLastError()));
		closesocket(serverSock);
		return 0;
	}

	listen(serverSock, 1);

	info("Listening on 0.0.0.0:4000");

	SOCKET client = accept(serverSock, NULL, NULL);
	if (client == SOCKET_ERROR)
	{
		error("Client error");
	}

	success("Client connected!");

	//Creating and getting a handle to the dump file
	HANDLE newFile = CreateFile(L".\\procdump.bin", GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	//Creating varuables for dumping the data
	char buffer[1024]; //buffer = 1kb
	int bytesRead = 0; //Keeps track of how many new bytes are in the buffer for WriteFile
	DWORD bytesWritten = 0; //Not used but required for WriteFile

	info("Recieving data");
	while ((bytesRead = recv(client, buffer, sizeof(buffer), 0)) > 0) //0 signals socket closed
	{
		WriteFile(newFile, buffer, bytesRead, &bytesWritten, NULL); //Appends data to the file
	}

	if (bytesRead == -1) //Recv returned an error
	{
		error("Recv error " + std::to_string(GetLastError()));
	}

	//Cleanup
	CloseHandle(newFile);
	closesocket(client);
	closesocket(serverSock);
	WSACleanup();

	return 0;
}
