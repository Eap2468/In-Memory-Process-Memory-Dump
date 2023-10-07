#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <arpa/inet.h>

#define success(str) std::cout << "[+] " << str << std::endl
#define error(str) std::cout << "[-] " << str << std::endl
#define info(str) std::cout << "[*] " << str << std::endl

int main()
{
	const char* ip = "0.0.0.0";
	int port = 4000;

	info("Creating dump file");
	int filefd = open("./dump.bin", O_WRONLY | O_APPEND | O_CREAT, 0644);

	//Creating socket
	int serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	//Binding socket to port
	info("Socket creating, attempting to bind to port");
	if(bind(serverfd, (sockaddr*)&addr, sizeof(addr)) != 0)
	{
		error("Bind error");
		close(filefd);
		return 0;
	}
	success("Port binded, listening on 0.0.0.0:4000");

	listen(serverfd, 1);
	info("Listening for connections");
	int clientfd = accept(serverfd, NULL, NULL);

	success("Client connected!");

	char buffer[1024];
	int bytes_recieved = 0, bytes_written;
	while((bytes_recieved = recv(clientfd, buffer, sizeof(buffer), 0)) > 0) //0 means socket was closed
	{
		bytes_written = write(filefd, buffer, bytes_recieved);
		if (bytes_written != bytes_recieved) //Checks that bytes were written properly, since this is a demo I don't have something to fix it though it could easily be done
		{
			error("File write error");
			break;
		}
	}
	success("Dump file should be written");
	close(filefd);
	close(clientfd);
	close(serverfd);

	return 0;
}
