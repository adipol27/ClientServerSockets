/*Authors Eli Harel- 209625623 and  Adi Polak- 308552967.
Project – Exercise 4
Description- In the lasr  exercise we practise working with several threads, processes, mutexes and semaphores.
In addition we practise comunication between several computers by managing a game of rock, paper, scissors, lizard and spock between clients.
*/

// Includes --------------------------------------------------------------------
#include "Server.h"
#include <stdio.h>
#include <string.h>
#include <math.h>  
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
// Constants -------------------------------------------------------------------
#define NUM_THREADS 13
#define ERROR_CODE ((int)(-1))
#define SUCCESS_CODE ((int)(0))
/*
•Description – This main function checks if the arguments given are valid, and sends the argv to "MainServer"
• Parameters –	argc- number of cells in the argv array, argv- location of an array containing the information: "<port>"
• Returns –returns ERROR_CODE if not succesful and SUCCESS_CODE if its succeded.
*/
int main(int argc, char *argv[])
{
	if (mainargumentstest(argc) == ERROR_CODE)			// main arguments invalid
	{
		printf("Wrong number of arguments");
		goto end;
	}
	MainServer(argv[1]);
end:
	return SUCCESS_CODE;
}

/*
•Description –checks if the argv is valid
• Parameters –argc- number of arguments in argv
• Returns –0 if not valid, 1 if so
*/
int mainargumentstest(int argc)
{
	if (argc < 2)
	{
		return(ERROR_CODE);
	}
	if (argc > 2)
	{
		return(ERROR_CODE);
	}
	return SUCCESS_CODE;

}