/*Authors Eli Harel- 209625623 and  Adi Polak- 308552967.
Project – Exercise 4
Description- In the last exercise we practise working with several threads, processes, mutexes and semaphores.
We practise comunication between several Clients and a Server by managing a game of rock, paper, scissors, lizard and spock.
*/

// Includes --------------------------------------------------------------------
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include "Client.h"
#include "SocketTools.h"


/*
•Description –This is the main client operation. The function manages the comunication from the client side.   
/////////////////////////////////////////////////
• Parameters – argv- location of an array containing the information: " <server ip> <server port> <username>"
• Returns –void
*/
void MainClient(char *argv[])
{
	SOCKET m_socket;
	SOCKET* m_socketp = &m_socket;
	SOCKADDR_IN clientService;
	char *AcceptedStr = NULL;
	char SendClientRequest[37];
	WSADATA wsaData;
	int retval=0;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		goto Cleanup2;
	}
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(argv[1]);
	clientService.sin_port = htons(atoi(argv[2]));
	if (Connect_Server(m_socketp, clientService, argv[2],1,argv[1]) == 0)
		goto Cleanup2;
	BuildRequest(SendClientRequest, argv[3],"CLIENT_REQUEST:");
Connected:
	printf("Connected to server on %s:%s.\n", argv[1], argv[2]);
	AcceptedStr = NULL;
	if (TransferResult(SendString(SendClientRequest, *m_socketp),"Send")==1)
	{
		if (Connect_Server(m_socketp, clientService, argv[2], 2,argv[1]) == 0)
			goto Cleanup2;
		else
			goto Connected;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, *m_socketp), "Receive") == 1)
	{
		free(AcceptedStr);
		if (Connect_Server(m_socketp, clientService, argv[2], 2,argv[1]) == 0)
			goto Cleanup2;
		else
			goto Connected;
	}
	if (MessageType(AcceptedStr, "SERVER_DENIED") == 0)
	{
		free(AcceptedStr);
		if (Connect_Server(m_socketp, clientService, argv[2], 3,argv[1]) == 0)
			goto Cleanup2;
		else
			goto Connected;
	}
	if (MessageType(AcceptedStr, "SERVER_APPROVED") == 0)
	{
		free(AcceptedStr);
		AcceptedStr = NULL;
		if (TransferResult(ReceiveString(&AcceptedStr, *m_socketp), "Receive") == 1)
		{
			free(AcceptedStr);
			if (Connect_Server(m_socketp, clientService, argv[2], 2,argv[1]) == 0)
				goto Cleanup2;
			else
				goto Connected;
		}
		if (MessageType(AcceptedStr, "SERVER_MAIN_MENU") == 0)
		{
			free(AcceptedStr);
			retval = MainMenu(*m_socketp);
			if (retval == 1)
			{
				if (Connect_Server(m_socketp, clientService, argv[2], 2,argv[1]) == 0)
					goto Cleanup2;
				else
					goto Connected;
			}
			if (retval == 2)
				goto Cleanup;
			
		}

	}

Cleanup:
	if (closesocket(m_socket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
Cleanup2:	
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	return;
}

/*
•Description –This function runs the connecting to the server. It manages cases like if the first connection failed, if it dropped the connection or denied connection. 
It then askes the client what he would like to do next and acts accordingly if he wants to quit or try to recconct
• Parameters – SOCKET* m_socketp, SOCKADDR_IN clientService ,char *argv- string of the port ,int type- What type of connection lost was it ,char* server- server ip
• Returns – 0 if the connection failed , 1 if it succeeded
*/
int Connect_Server(SOCKET* m_socketp, SOCKADDR_IN clientService,char *argv,int type ,char* server)
{
	BOOL answered = FALSE;
	int answer = 0;
	if(type==1)
	{
		*m_socketp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (*m_socketp == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			return 0;
		}
		while (TRUE)
		{
			if (connect(*m_socketp, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
			{
				printf("Failed connecting to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", server, argv);
				while (!answered)
				{
					scanf_s("%2d", &answer);
					clean_stdin();
					if (answer == 2)
						return 0;
					if (answer == 1)
						answered = TRUE;
					else	
						printf("Invalid Response, Respond Again\n");
				
				}
				answered = FALSE;
			}	
			else
				return 1;
		}
	}
	else if (type==2)
	{
		printf("Connection to server on %s:%s has been lost.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", server, argv);
		closesocket(*m_socketp);
		while (!answered)
		{
			scanf_s("%2d", &answer);
			clean_stdin();
			if (answer == 2)
			{
				closesocket(*m_socketp);
				return 0;
			}
			if (answer == 1)
				answered = TRUE;
			else
				printf("Invalid Response, Respond Again\n");

		}
		return Connect_Server(m_socketp, clientService, argv, 1,server);
	}
	else
	{
		printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", server, argv);
		while (!answered)
		{
			scanf_s("%2d", &answer);
			clean_stdin();
			if (answer == 2)
			{
				closesocket(*m_socketp);
				return 0;
			}
			if (answer == 1)
				answered = TRUE;
			else
				printf("Invalid Response, Respond Again\n");

		}
		closesocket(*m_socketp);
		return Connect_Server(m_socketp, clientService, argv, 1, server);
	}
}
/*
•Description –The function runs the Main Menu to the client. It prints the options, recieves the answer and sends the information recieved accordingly.
• Parameters – SOCKET socket- 
• Returns – 2- to disconnect, 1 - to continue (in case there are no errors), MainMenu(socket)- this is a reccursive function, if the client wants to return to main menu it cleans the data and calls itself.
*/

int MainMenu(SOCKET socket)
{
	char *AcceptedStr = NULL;
	int answer = 0;
	int retval = -1;
	printf("Choose what to do next:\n1. Play against another client\n2. Play against the server\n3. View the leaderboard\n4. Quit\n");
	while (TRUE)
	{
		scanf_s("%2d", &answer);
		clean_stdin();
		if (answer == 4)
			return 2;
		if (answer==2)
		{
			if (TransferResult(SendString("CLIENT_CPU\n", socket), "Send") == 1)
				return 1;
			if (Game(socket)==1)
				return 1;
			if (TransferResult(SendString("CLIENT_MAIN_MENU\n", socket), "Send") == 1)
				return 1;
			if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
				return 1;
			if (MessageType(AcceptedStr, "SERVER_MAIN_MENU") == 0)
			{
				free(AcceptedStr);
				return MainMenu(socket);
			}
			free(AcceptedStr);
			return 1;
		}
		if (answer == 1)
		{
			if (TransferResult(SendString("CLIENT_VERSUS\n", socket), "Send") == 1)
				return 1;
			retval = VersusGame(socket);
			if (retval != 2)
			{
				if (retval==1)
					return 1;
				if (retval==0)
					if (TransferResult(SendString("CLIENT_MAIN_MENU\n", socket), "Send") == 1)
						return 1;
			}
			if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
			{
				free(AcceptedStr);
				return 1;
			}
			if (MessageType(AcceptedStr, "SERVER_MAIN_MENU") == 0)
			{
				free(AcceptedStr);
				return MainMenu(socket);
			}
			free(AcceptedStr);
			return 1;
		}
		else
			printf("Invalid Response Try Again\n");
	}
}
/*
•Description – The Game function runs CPUGame  in the client side. It checks the massage reseived from the server. if its "SERVER_PLAYER_MOVE_REQUEST", it prints the game instructions, recieves data from the client (changes the string to upper case) and sends the string,
if it is "SERVER_GAME_RESULTS", it prints the results and areturns to "Game", if it is "SERVER_GAME_OVER_MENU" it asks what to do next and acts accordingly.
• Parameters – SOCKET socket
• Returns – 0- return to main menu, 1 - there was an error,  Game(SOCKET socket)- play again
*/

int Game(SOCKET socket)
{
	char* AcceptedStr = NULL;
	char str[10];
	BOOL quit = FALSE;
	char SendStr[29];
	char results[46];
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
	{
		free(AcceptedStr);
		return 1;
	}
	if (MessageType(AcceptedStr, "SERVER_PLAYER_MOVE_REQUEST") == 0)
	{
		free(AcceptedStr);
		printf("Choose a move from the list: Rock, Paper, Scissors, Lizard or Spock:");
		while (!quit)
		{
			scanf_s("%9s", str, 10);
			clean_stdin();
			UpperCase(str);
			if (InputValid(str)==0)
			{
				BuildRequest(SendStr, str, "CLIENT_PLAYER_MOVE:");
				if (TransferResult(SendString(SendStr, socket), "Send") == 1)
					return 1;
				return Game(socket);
			}
			printf("INVALID INPUT:Choose a move from the list.\n");
		}
	}
	if (MessageType(AcceptedStr, "SERVER_GAME_RESULTS") == 0)
	{
		MessageInfo(AcceptedStr, results);
		free(AcceptedStr);
		PrintResults(results);
		return Game(socket);
	}
	if (MessageType(AcceptedStr, "SERVER_GAME_OVER_MENU") == 0)
	{
		BOOL answered = FALSE;
		int answer = 0;
		free(AcceptedStr);
		printf("Choose what to do next:\n1. Play again\n2. Return to the main menu\n");
		while (!answered)
		{
			scanf_s("%2d", &answer);
			clean_stdin();
			if (answer == 1)
			{
				if (TransferResult(SendString("CLIENT_REPLAY\n", socket), "Send") == 1)
					return 1;
				return Game(socket);
			}
			if (answer == 2)
				return 0;
			else
				printf("Invalid Response Try Again\n");
		}
	}
	return 1;
}
/*
•Description –UpperCase turn a general string to all upper case letters
• Parameters –char* s- input string 
• Returns –void
*/
//upper-cases player's move
void UpperCase(char* s)
{
	int i = 0;
	for (i = 0; s[i] != '\0'; i++)
		if (s[i] >= 'a' && s[i] <= 'z')
			s[i] = s[i] - 32;
}
/*
•Description –Checks the validity of the users input
• Parameters –char* str- input string from the client  
• Returns –0 if valid, 1 if not
*/

int InputValid(char* str)
{
	if (STRINGS_ARE_EQUAL(str, "ROCK") || STRINGS_ARE_EQUAL(str, "PAPER")
		|| STRINGS_ARE_EQUAL(str, "SCISSORS") || STRINGS_ARE_EQUAL(str, "LIZARD") || STRINGS_ARE_EQUAL(str, "SPOCK"))
		return 0;
	return 1;
}
/*
•Description –This function extracts the game results message from the input information and prints it.
• Parameters –char *str- a string with all the information about who won
• Returns –void
*/

void PrintResults(char *str)
{
	char rival[21];
	char rivalmove[9];
	char yourmove[9];
	char winner[21];
	char* results[4] = {rival,rivalmove,yourmove,winner};
	int i = 0;
	int j = 0;
	int k = 0;
	while( str[i-1] != '\0')
	{
		while (str[i] != ';' && str[i]!='\0')
		{
			results[k][j] = str[i];
			i++;
			j++;
		}
		results[k][j] = '\0';
		i++;
		k++;
		j = 0;
	}
	if (results[3][0]=='\0')
		printf("You played: %s\n%s played: %s\n",yourmove,rival,rivalmove);
	else
		printf("You played: %s\n%s played: %s\n%s won!\n", yourmove, rival, rivalmove, winner);
}
/*
•Description –Runs the game between  users from the client side.
• Parameters –SOCKET socket- 
• Returns – 2- no opponent, 1- error , 0- return to main menu, VersusGame(socket)- try to play againts another client again
*/
int VersusGame(SOCKET socket)
{
	char *AcceptedStr = NULL;
	int retval = 0;
	char str[10];
	BOOL quit = FALSE;
	char SendStr[29];
	char results[46];
	char rival_username[22];
	FD_SET ReadSet;
	FD_ZERO(&ReadSet);
	FD_SET(socket, &ReadSet);
	if (select(0, &ReadSet, NULL, NULL, &time_wait) == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return 1;
	}
	if (TransferResult(ReceiveString(&AcceptedStr, socket), "Receive") == 1)
	{
		free(AcceptedStr);
		return 1;
	}
	if (MessageType(AcceptedStr, "SERVER_OPPONENT_QUIT") == 0)
	{
		MessageInfo(AcceptedStr, rival_username);
		printf("%s has left the game!\n",rival_username);
		free(AcceptedStr);
		return 2;
	}
	if (MessageType(AcceptedStr, "SERVER_NO_OPPONENTS") == 0)
	{
		printf("No opponents found\n");
		free(AcceptedStr);
		return 2;
	}
	if (MessageType(AcceptedStr, "SERVER_INVITE") == 0)
	{
		free(AcceptedStr);
		return VersusGame(socket);
	}
	if (MessageType(AcceptedStr, "SERVER_PLAYER_MOVE_REQUEST") == 0)
	{
		free(AcceptedStr);
		printf("Choose a move from the list: Rock, Paper, Scissors, Lizard or Spock:");
		while (!quit)
		{
			scanf_s("%9s", str, 10);
			clean_stdin();
			UpperCase(str);
			if (InputValid(str) == 0)
			{
				BuildRequest(SendStr, str, "CLIENT_PLAYER_MOVE:");
				if (TransferResult(SendString(SendStr, socket), "Send") == 1)
					return 1;
				FD_ZERO(&ReadSet);
				FD_SET(socket, &ReadSet);
				if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
				{
					printf("select() failed, error %d\n", WSAGetLastError());
					return 1;
				}
				return VersusGame(socket);
			}
			printf("INVALID INPUT:Choose a move from the list.\n");
		}
	}
	if (MessageType(AcceptedStr, "SERVER_GAME_RESULTS") == 0)
	{
		MessageInfo(AcceptedStr, results);
		free(AcceptedStr);
		PrintResults(results);
		return VersusGame(socket);
	}
	if (MessageType(AcceptedStr, "SERVER_GAME_OVER_MENU") == 0)
	{
		BOOL answered = FALSE;
		int answer = 0;
		free(AcceptedStr);
		printf("Choose what to do next:\n1. Play again\n2. Return to the main menu\n");
		while (!answered)
		{
			scanf_s("%2d", &answer);
			clean_stdin();
			if (answer == 1)
			{
				if (TransferResult(SendString("CLIENT_REPLAY\n", socket), "Send") == 1)
					return 1;
				return VersusGame(socket);
			}
			if (answer == 2)
				return 0;
			else
				printf("Invalid Response Try Again\n");
		}
	}
	free(AcceptedStr);
	return 1;
}