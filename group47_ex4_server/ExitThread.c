/*Authors Eli Harel- 209625623 and  Adi Polak- 308552967.
Project – Exercise 4
Description- In the last exercise we practise working with several threads, processes, mutexes and semaphores.
We practise comunication between several Clients and a Server by managing a game of rock, paper, scissors, lizard and spock.
*/

// Includes --------------------------------------------------------------------
#include <winsock2.h>
#include <windows.h>
#include "Server.h"
#include "SocketTools.h"
#include <stdio.h>
#include <string.h>
#include <math.h>  
#include <stdbool.h>




/*
•Description –QuitThread is a thread that manages if "exit" was writen to the commandline of the server.
• Parameters –void
• Returns –0 if successful
*/
DWORD QuitThread(void)
{
	char str[6];
	while (TRUE)
	{
		scanf_s("%5s",str,6);
		clean_stdin();
		if (str[4]=='\0' && STRINGS_ARE_EQUAL(str, "exit"))
		{
			return 0;
		}
		printf("INVALID INPUT:To quit type 'exit'\n");
	}
}