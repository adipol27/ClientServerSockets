/*Authors Eli Harel- 209625623 and  Adi Polak- 308552967.
Project – Exercise 4
Description- In the last exercise we practise working with several threads, processes, mutexes and semaphores.
We practise comunication between several Clients and a Server by managing a game of rock, paper, scissors, lizard and spock.
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <stdlib.h>
#include "Server.h"
#include "SocketTools.h"
#include <tchar.h>
#include <io.h>
// Constants -------------------------------------------------------------------

#define NUM_OF_WORKER_THREADS 2
#define MAX_LOOPS 3
#define SEND_STR_SIZE 35
static LPCTSTR MUTEX_NAME = _T("Named_Mutex__Versus_Game");
static LPCTSTR SEMAPHORE_NAME = _T("Named_Semaphore__Versus_Game");
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
HANDLE QuitThreadHandle;
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
int count = 0;

/*
•Description –This function manages the main operation of the server
• Parameters – char *port- The port of the server
• Returns – void
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS

void MainServer(char *port)
{
	int Ind;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	int retval = -1;
	DWORD exitcode;
	FD_SET ReadSet;
	TransferResult_t SendRes;
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());                               
		return;
	}  
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",SERVER_ADDRESS_STR);
		goto server_cleanup_2;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(atoi(port));
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;
	printf("Waiting for a client to connect...\n");
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}
	QuitThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)QuitThread, &(ThreadInputs[Ind]), 0, NULL);
	while(TRUE)
	{
Select:		
		FD_ZERO(&ReadSet);
		FD_SET(MainSocket, &ReadSet);
		retval = select(0, &ReadSet, NULL, NULL, &no_wait);
		if (retval == 0)
		{
			GetExitCodeThread(QuitThreadHandle, &exitcode);
			if (exitcode != STILL_ACTIVE)
			{
				printf("Server Exiting\n");
				CloseHandle(QuitThreadHandle);
				goto server_cleanup_3;
			}
		}
		else if (retval == SOCKET_ERROR)
		{
			printf("select() failed, error %d\n", WSAGetLastError());
			goto server_cleanup_4;
		}
		else
		{
			SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
				goto Select;
			}
			printf("Client Connected.\n");
			Ind = FindFirstUnusedThreadSlot();
			if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
			{
				SendRes = SendString("SERVER_DENIED:No slots available for you, dropping the connection.\n", AcceptSocket);
				if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing.\n");
					closesocket(AcceptSocket);
					goto Select;
				}
				Sleep(500);
				closesocket(AcceptSocket); //Closing the socket, dropping the connection.
				printf("Client Rejected: Server Full\n");
			}
			else
			{
				ThreadInputs[Ind] = AcceptSocket;
				ThreadHandles[Ind] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ServiceThread,&(ThreadInputs[Ind]),0,NULL);
			}
		}
	} 

server_cleanup_4:
	TerminateThread(QuitThreadHandle, (DWORD)-1);
	CloseHandle(QuitThreadHandle);
server_cleanup_3:
	CleanupWorkerThreads();
	if (_access_s(GAME_SESSION, 00) == 0)
	{
		remove(GAME_SESSION);
	}
server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	return;
}
/*
•Description – The function searches for an empty slot for a thread and returns it
• Parameters –void
• Returns – Ind- the index of the empty slot 
*/
static int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);
			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}
/*
•Description –This function function closes the working sockets and handles and terminate the threads   
• Parameters –void
• Returns –void
*/
static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 15000);
			closesocket(ThreadInputs[Ind]);
			if (Res == WAIT_OBJECT_0)

				CloseHandle(ThreadHandles[Ind]);
			else
				TerminateThread(ThreadHandles[Ind],(DWORD)(-1));
			ThreadHandles[Ind] = NULL;
		}
	}

}
/*
•Description –This function accepts the client and manages the massages it returns and the actions in return.
• Parameters – SOCKET *t_socket
• Returns – 0 for success
*/
static DWORD ServiceThread(SOCKET *t_socket)
{
	BOOL Done = FALSE;
	char *AcceptedStr = NULL;
	char Username[21];
	char SendStr[27];
	if (TransferResult(ReceiveString(&AcceptedStr, *t_socket),"Receive") == 1)
		goto CleanupR;
	if (MessageType(AcceptedStr, "CLIENT_REQUEST") != 0)
	{
		printf("CLIENT_REQUEST not sent\n");
		goto CleanupR;
	}
	MessageInfo(AcceptedStr, Username);
	free(AcceptedStr);
	AcceptedStr = NULL;
	strcpy(SendStr, "SERVER_APPROVED\n");
	if (TransferResult(SendString(SendStr, *t_socket), "Send") == 1)
		goto CleanupS;
	strcpy(SendStr, "SERVER_MAIN_MENU\n");
	if (TransferResult(SendString(SendStr, *t_socket), "Send") == 1)
		goto CleanupS;
	if (MainMenu(*t_socket,Username) == 1)
		goto CleanupR;


CleanupR:
	free(AcceptedStr);
CleanupS:	
	closesocket(*t_socket);
	return 0;
}
/*
•Description –The MainMenu function deals with the main menu from the server side. It recieves massages from the client and acts accordingly.
• Parameters –SOCKET socket,char* Username- the user name string
• Returns – 1 if successful, MainMenu(socket, Username)- if it need to run the Main menu again
*/
int MainMenu(SOCKET socket,char* Username)
{
	char* AcceptedStr = NULL;
	FD_SET ReadSet;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		goto End;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
		goto End;
	if (MessageType(AcceptedStr, "CLIENT_DISCONNECT") == 0)
		goto End;
	if (MessageType(AcceptedStr, "CLIENT_CPU") == 0)
	{
		if (CPUGame(socket,Username) == 1)
			goto End;
		if (TransferResult(SendString("SERVER_MAIN_MENU\n", socket), "Send") == 1)
			goto End;
		free(AcceptedStr);
		return MainMenu(socket, Username);
	}
	if (MessageType(AcceptedStr, "CLIENT_VERSUS") == 0)
	{
		if (VersusGame(socket, Username,NULL) == 1)
			goto End;
		if (TransferResult(SendString("SERVER_MAIN_MENU\n", socket), "Send") == 1)
			goto End;
		free(AcceptedStr);
		return MainMenu(socket, Username);
	}
End:
	free(AcceptedStr);
	return 1;
}
/*
•Description – Manages the cpu game from the server side. It recieves the moves from the client, randomly chooses a move and sends who won to the client. 
• Parameters –SOCKET socket , char* username- string of the username
• Returns –1 if failed, CPUGame(socket,username)- if the client wants to play again
*/
int CPUGame(SOCKET socket, char* username)
{
	char* Moves[5] = {"ROCK","PAPER","SCISSORS","LIZARD","SPOCK"};
	char* CpuMove = Moves[rand_lim(4)];
	char UserMove[9];
	char SendStr[67];
	char *AcceptedStr = NULL;
	FD_SET ReadSet;
	strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		return 1;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return 1;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
	{
		free(AcceptedStr);
		return 1;
	}
	if (MessageType(AcceptedStr, "CLIENT_PLAYER_MOVE") != 0)
	{
		printf("CLIENT_PLAYER_MOVE not sent\n");
		return 1;
	}
	MessageInfo(AcceptedStr, UserMove);
	free(AcceptedStr);
	AcceptedStr = NULL;
	WhoWon(CpuMove, UserMove,"Server",username,SendStr);
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		return 1;
	strcpy(SendStr, "SERVER_GAME_OVER_MENU\n");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		return 1;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return 1;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
		return 1;
	if (MessageType(AcceptedStr, "CLIENT_REPLAY") == 0)
	{
		return CPUGame(socket,username);
	}
	if (MessageType(AcceptedStr, "CLIENT_MAIN_MENU") == 0)
	{
		return 0;
	}
	return 1;
}
/*
•Description –This function finds the winner of a game and creates the message to be sent to to client
• Parameters – char* Other_player- a string of the other player, char* This_player- a string of the first player , char* other_username- string of the other username ,char* you- string of one of the  usernames,
char* result- a pointer in which the function will put the output result
• Returns –void
*/

void WhoWon(char* Other_player, char* This_player , char* other_username ,char* you, char* result)
{
	int i = -1; int j = 0; int k = 0; int l = 0; int m = 0; int n = 0;
	char* desc = "SERVER_GAME_RESULTS:";
	if (STRINGS_ARE_EQUAL(Other_player, "ROCK"))
	{
		if (STRINGS_ARE_EQUAL(This_player,"LIZARD") || STRINGS_ARE_EQUAL(This_player,"SCISSORS"))
			i=1;
		else
			i=2;
	}
	if (STRINGS_ARE_EQUAL(Other_player, "PAPER"))
	{
		if (STRINGS_ARE_EQUAL(This_player,"ROCK") || STRINGS_ARE_EQUAL(This_player,"SPOCK"))
			i=1;
		else
			i=2;
	}
	if (STRINGS_ARE_EQUAL(Other_player, "SCISSORS"))
	{
		if (STRINGS_ARE_EQUAL(This_player,"PAPER") || STRINGS_ARE_EQUAL(This_player,"LIZARD"))
			i=1;
		else
			i=2;
	}
	if (STRINGS_ARE_EQUAL(Other_player, "LIZARD"))
	{
		if (STRINGS_ARE_EQUAL(This_player,"SPOCK") || STRINGS_ARE_EQUAL(This_player,"PAPER"))
			i=1;
		else
			i=2;
	}
	if (STRINGS_ARE_EQUAL(Other_player, "SPOCK"))
	{
		if (STRINGS_ARE_EQUAL(This_player,"ROCK") || STRINGS_ARE_EQUAL(This_player,"SCISSORS"))
			i=1;
		else
			i=2;
	}
	if (STRINGS_ARE_EQUAL(This_player,Other_player))
		i = 0;
	while (desc[j] != '\0')
	{
		result[j] = desc[j];
		j++;
	}
	result[j] = ':';
	while (other_username[k] != '\0')
	{
		result[j] = other_username[k];
		j++;
		k++;
	}
	result[j] = ';';
	j++;
	while (Other_player[l] != '\0')
	{
		result[j] = Other_player[l];
		j++;
		l++;
	}
	result[j] = ';';
	j++;
	while (This_player[m] != '\0')
	{
		result[j] = This_player[m];
		j++;
		m++;
	}
	result[j] = ';';
	j++;
	if (i == 1)
	{
		while (other_username[n] != '\0')
		{
			result[j] = other_username[n];
			j++;
			n++;
		}
	}
	if (i==2)
	{
		while (you[n] != '\0')
		{
			result[j] = you[n];
			j++;
			n++;
		}
	}
	result[j] = '\n';
	result[j + 1] = '\0';
}
/*
•Description –The VersusGame manages a game between two clients from the server side, it opens a txt file and uses it to write and read the usernames and moves. In addition it checks for errors and finally closes handels when not needed.
• Parameters –SOCKET socket, char* username- clients username string, char* prev_riva- the previous rivals name string
• Returns –0- no opponent, 1- success, VersusGame(socket, username,rival_username)- another game.
*/
int VersusGame(SOCKET socket, char* username, char* prev_rival)
{
	char SendStr[81], UserMove[9], RivalMove[9], rival_username[22];
	int retval = -1;
	HANDLE mutex_handle;
	HANDLE semaphore_handle;
	FILE* fp = NULL;
	char *AcceptedStr = NULL;
	BOOL am_opener = FALSE, DONE = FALSE;
	FD_SET ReadSet;
	mutex_handle = CreateMutexA(NULL, FALSE, MUTEX_NAME);
	if (NULL == mutex_handle)
	{
		printf("Error when creating mutex: %d\n", GetLastError());
		goto Cleanup2;
	}
	semaphore_handle = CreateSemaphoreA(NULL, 0, 2, SEMAPHORE_NAME);
	if (NULL == semaphore_handle)
	{
		printf("Error when creating semaphore: %d\n", GetLastError());
		goto Cleanup;
	}
	retval = CreateGame(mutex_handle, semaphore_handle, username, rival_username, &am_opener, socket, prev_rival);
	if (retval == 1)
		goto Cleanup;
	if (retval == 2)
		return 0;
	strcpy(SendStr, "SERVER_INVITE\n");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		goto Cleanup;
	strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		goto Cleanup;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		goto Cleanup;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
		goto Cleanup;
	if (MessageType(AcceptedStr, "CLIENT_PLAYER_MOVE") != 0)
		goto Cleanup;
	MessageInfo(AcceptedStr, UserMove);
	free(AcceptedStr);
	AcceptedStr = NULL;
	if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
	{
		printf("Error when waiting for mutex\n");
		goto Cleanup;
	}
	count++;
	if (count == 2)
	{
		count = 0;
		if (FALSE == ReleaseSemaphore(semaphore_handle, 2, NULL))
		{
			printf("Error when releasing semaphore\n");
			goto Cleanup;
		}
	}
	if (ReleaseMutex(mutex_handle) == 0)
	{
		printf("Error when releasing mutex\n");
		goto Cleanup;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(semaphore_handle, INFINITE))
	{
		printf("Error when waiting for semaphore\n");
		goto Cleanup;;
	}
	if (WriteandGetRival(am_opener,mutex_handle,semaphore_handle,UserMove,RivalMove) == 1)
		goto Cleanup;
	WhoWon(RivalMove, UserMove, rival_username, username, SendStr);
	if (am_opener == TRUE)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
		{
			printf("Error when waiting for mutex\n");
			goto Cleanup;
		}
		remove(GAME_SESSION);
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			goto Cleanup;
		}
	}
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		goto Cleanup;
	strcpy(SendStr, "SERVER_GAME_OVER_MENU\n");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		goto Cleanup;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		goto Cleanup;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
		goto Cleanup;
	if (MessageType(AcceptedStr, "CLIENT_REPLAY") == 0)
	{
		CloseHandle(semaphore_handle);
		CloseHandle(mutex_handle);
		free(AcceptedStr);
		return VersusGame(socket, username,rival_username);
	}
	if (MessageType(AcceptedStr, "CLIENT_MAIN_MENU") == 0)
	{
		CloseHandle(semaphore_handle);
		CloseHandle(mutex_handle);
		free(AcceptedStr);
		return 0;
	}
Cleanup:
	CloseHandle(semaphore_handle);
Cleanup2:
	CloseHandle(mutex_handle);
	if (_access_s(GAME_SESSION, 00) == 0)
	{
		remove(GAME_SESSION);
	}
	return 1;
}
/*
•Description –This function chooses a number randomly betwwen 1 and "limit".
• Parameters –limit- the max value for the resule
• Returns –retval- the random value 
*/
// rand num generator up to lim int
int rand_lim(int limit)
{
	int divisor = RAND_MAX / (limit + 1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval;
}
/*
•Description –WriteToFile writes the "writestr" to the file in a specific format (with '\n' character at end)
• Parameters –(FILE* fp- the file pointer, char* writestr- a string which is writen to the file
• Returns –void
*/
void WriteToFile(FILE* fp, char* writestr)
{
	int i = 0;
	char tofile[36];
	while (writestr[i] != '\0')
	{
		tofile[i] = writestr[i];
		i++;
	}
	tofile[i] = '\n';
	tofile[i + 1] = '\0';
	fputs(tofile, fp);
}
/*
•Description – If there are no opponents for vs game (2 clients game), the function takes care of relevant needs, closes game session and sends suitable messages to clients 
• Parameters – DWORD wait_code,HANDLE mutex_handle, HANDLE semaphore_handle,SOCKET socket, char* prev_rival- string of the previous rival
• Returns – 1 if failed, 0 if succeded
*/
int No_Opponents(DWORD wait_code,HANDLE mutex_handle, HANDLE semaphore_handle,SOCKET socket, char* prev_rival)
{
	char SendStr[46];
	if (wait_code != WAIT_TIMEOUT)
	{
		printf("Error when waiting for semaphore\n");
		return 1;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
	{
		printf("Error when waiting for mutex\n");
		return 1;
	}
	if (remove(GAME_SESSION) != 0)
		return 1;
	if (ReleaseMutex(mutex_handle) == 0)
	{
		printf("Error when releasing mutex\n");
		return 1;
	}
	if (prev_rival == NULL)
		strcpy(SendStr, "SERVER_NO_OPPONENTS\n");
	else
		BuildRequest(SendStr, prev_rival, "SERVER_OPPONENT_QUIT:");
	if (TransferResult(SendString(SendStr, socket), "Send") == 1)
		return 1;
	CloseHandle(mutex_handle);
	CloseHandle(semaphore_handle);
	return 0;
}
/*
•Description –This function write the callers move to the text file and when entering the text file it extracts also the rivals name.
• Parameters –BOOL am_opener- a boolian which determain if the caller to the function created the text file or not, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove- string of the users move , char* RivalMove- string of rivals move
• Returns – 1 if failed, 0 if succeded
*/

int WriteandGetRival(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove, char* RivalMove)
{
	if (WriteMoves(am_opener, mutex_handle, semaphore_handle, UserMove) == 1)
		return 1;
	if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
	{
		printf("Error when waiting for mutex\n");
		return 1;
	}
	count++;
	if (count == 2)
	{
		count = 0;
		if (FALSE == ReleaseSemaphore(semaphore_handle, 2, NULL))
		{
			printf("Error when releasing semaphore\n");
			return 1;
		}
	}
	if (ReleaseMutex(mutex_handle) == 0)
	{
		printf("Error when releasing mutex\n");
		return 1;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(semaphore_handle, INFINITE))
	{
		printf("Error when waiting for semaphore\n");
		return 1;
	}
	if (ReadMoves(am_opener, mutex_handle, semaphore_handle, RivalMove) == 1)
		return 1;
	return 0;
}
/*
•Description –This function writes the users move to gamesession. the "am_opener" boolian determance if the function should write first or wait for the other thread to write
• Parameters –BOOL am_opener-a boolian which determain if the caller to the function created the text file or not, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove- string of users move
• Returns – 1 if faild , 0 if succeded
*/
int WriteMoves(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove)
{
	FILE* fp = NULL;
	if (am_opener == TRUE)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
		{
			printf("Error when waiting for mutex\n");
			return 1;
		}
		fp = fopen(GAME_SESSION, "a");
		if (fp == NULL)
		{
			printf("Error when opening file\n");
			return 1;
		}
		WriteToFile(fp, UserMove);
		fclose(fp);
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			return 1;
		}
		if (FALSE == ReleaseSemaphore(semaphore_handle, 1, NULL))
		{
			printf("Error when releasing semaphore\n");
			return 1;
		}
		return 0;
	}
	if (am_opener == FALSE)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(semaphore_handle, INFINITE))
		{
			printf("Error when waiting for semaphore\n");
			return 1;
		}
		if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
		{
			printf("Error when waiting for mutex\n");
			return 1;
		}
		fp = fopen(GAME_SESSION, "a");
		fputs(UserMove, fp);
		fclose(fp);
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			return 1;
		}
		return 0;
	}
	return 1;
}
/*
•Description –This function gets the rival's move for gamesession text and copies it to "RivalMove"
• Parameters –BOOL am_opener-a boolian which determain if the caller to the function created the text file or not, HANDLE mutex_handle, HANDLE semaphore_handle, char* RivalMove- string of the rivals move
• Returns – 1 if failed , 0 if succeded
*/
int ReadMoves(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* RivalMove)
{
	FILE* fp = NULL;
	if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
	{
		printf("Error when waiting for mutex\n");
		return 1;
	}
	if (am_opener == TRUE)
	{
		fp = fopen(GAME_SESSION, "r");
		skip_line(fp); skip_line(fp); skip_line(fp);
		fgets(RivalMove, 10, fp);
		fclose(fp);
	}
	if (am_opener == FALSE)
	{
		fp = fopen(GAME_SESSION, "r");
		skip_line(fp); skip_line(fp);
		fgets(RivalMove, 10, fp);
		fclose(fp);
		RivalMove[strlen(RivalMove) - 1] = '\0';
	}
	if (ReleaseMutex(mutex_handle) == 0)
	{
		printf("Error when releasing mutex\n");
		return 1;
	}
	return 0;
}
/*
•Description – The CreateGame creats vsgame by creating a gamesession and writing usernames to it or callin "no_opponents"/
• Parameters –HANDLE mutex_handle, HANDLE semaphore_handle,char* username -string of username , char* rival_username- string of rivals username, BOOL* am_opener- which client called this function (if created the txt file or not), 
SOCKET socket, char* prev_rival- string of the previous rival user name.
• Returns – 1 if failed.  0- need to continue the game, 2- no opponents, back to main menu 
*/
int CreateGame(HANDLE mutex_handle, HANDLE semaphore_handle,char* username, char* rival_username, BOOL* am_opener, SOCKET socket, char* prev_rival)
{
	FILE* fp = NULL;
	DWORD wait_code;
	if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
	{
		printf("Error when waiting for mutex\n");
		return 1;
	}
	if (_access_s(GAME_SESSION, 00) != 0)
	{
		fp=fopen( GAME_SESSION, "w");
		if(fp==NULL)
		{
			printf("Error when creating file\n");
			return 1;
		}
		WriteToFile(fp, username);
		fclose(fp);
		*am_opener = TRUE;
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			return 1;
		}
		wait_code = WaitForSingleObject(semaphore_handle, 15000);
		if (wait_code != WAIT_OBJECT_0)
		{
			if (No_Opponents(wait_code, mutex_handle, semaphore_handle, socket, prev_rival) == 1)
				return 1;
			return 2;
		}
		if (WAIT_OBJECT_0 != WaitForSingleObject(mutex_handle, INFINITE))
		{
			printf("Error when waiting for mutex\n");
			return 1;
		}
		fp = fopen(GAME_SESSION, "r");
		if (fp == NULL)
		{
			printf("Error when opening file\n");
			return 1;
		}
		skip_line(fp);
		fgets(rival_username, 21, fp);
		fclose(fp);
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			return 1;
		}
		rival_username[strlen(rival_username) - 1] = '\0';
	}
	if (*am_opener == FALSE)
	{
		fp=fopen( GAME_SESSION, "r+");
		if (fp == NULL)
		{
			printf("Error when opening file\n");
			return 1;
		}
		fgets(rival_username, 22, fp);
		WriteToFile(fp, username);
		fclose(fp);
		if (FALSE == ReleaseSemaphore(semaphore_handle, 1, NULL))
		{
			printf("Error when releasing semaphore\n");
			return 1;
		}
		if (ReleaseMutex(mutex_handle) == 0)
		{
			printf("Error when releasing mutex\n");
			return 1;
		}
		rival_username[strlen(rival_username) - 1] = '\0';
	}
	return 0;
}