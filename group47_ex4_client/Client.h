// Includes --------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#ifndef CLIENT_H
#define CLIENT_H
#include <winsock2.h>
#include <windows.h>	

// Functions Decleration -------------------------------------------------------------------
int mainargumentstest(int argc);
void MainClient(char *argv[]);
int Connect_Server(SOCKET* m_socketp, SOCKADDR_IN clientService, char *argv, int type, char* server);
int MainMenu(SOCKET socket);
void UpperCase(char* s);
void PrintResults(char *str);
int InputValid(char* str);
int Game(SOCKET socket);
int VersusGame(SOCKET socket);
#endif