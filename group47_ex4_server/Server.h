
// Includes --------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef SERVER_H
#define SERVER_H
#define GAME_SESSION "./GameSession.txt"
#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

// Functions Decleration -------------------------------------------------------------------
int mainargumentstest(int argc);
void MainServer(char *port);
DWORD QuitThread(void);
int CPUGame(SOCKET socket, char* username);
int MainMenu(SOCKET socket, char* Username);
void WhoWon(char* Other_player, char* This_player, char* other_username, char* you, char* result);
int FindFirstUnusedThreadSlot();
void CleanupWorkerThreads();
int rand_lim(int limit);
int VersusGame(SOCKET socket, char* username, char* prev_rival);
void WriteToFile(FILE* fp, char* writestr);
int No_Opponents(DWORD wait_code, HANDLE mutex_handle, HANDLE semaphore_handle, SOCKET socket, char* prev_rival);
int WriteandGetRival(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove, char* RivalMove);
int WriteMoves(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* UserMove);
int ReadMoves(BOOL am_opener, HANDLE mutex_handle, HANDLE semaphore_handle, char* RivalMove);
int CreateGame(HANDLE mutex_handle, HANDLE semaphore_handle, char* username, char* rival_username, BOOL* am_opener, SOCKET socket, char* prev_rival);
DWORD ServiceThread(SOCKET *t_socket);
#endif