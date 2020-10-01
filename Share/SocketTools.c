
// Includes --------------------------------------------------------------------
#include "SocketTools.h"
#include <stdio.h>
#include <string.h>


/**
 * SendBuffer() uses a socket to send a buffer.
 *
 * Accepts:
 * -------
 * Buffer - the buffer containing the data to be sent.
 * BytesToSend - the number of bytes from the Buffer to send.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if sending succeeded
 * TRNS_FAILED - otherwise
 */

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}

		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/**
 * SendString() uses a socket to send a string.
 * Str - the string to send.
 * sd - the socket used for communication.
 */
TransferResult_t SendString(const char *Str, SOCKET sd)
{
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;
	FD_SET WriteSet;
	int retval=-1;

	TotalStringSizeInBytes = (int)(strlen(Str) + 1); // terminating zero also sent	
	FD_ZERO(&WriteSet);
	FD_SET(sd, &WriteSet);
	retval=select(0, NULL, &WriteSet, NULL, &time_wait);
	if (retval == 0)
	{
		printf("Connection Timed-out\n");
		return TRNS_FAILED;
	}
	if (retval == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return TRNS_FAILED;
	}
	SendRes = SendBuffer(
		(const char *)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // sizeof(int) 
		sd);

	if (SendRes != TRNS_SUCCEEDED) return SendRes;
	FD_ZERO(&WriteSet);
	FD_SET(sd, &WriteSet);
	retval = -1;
	retval = select(0, NULL, &WriteSet, NULL, &time_wait);
	if (retval == 0)
	{
		printf("Connection Timed-out\n");
		return TRNS_FAILED;
	}
	if (retval == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return TRNS_FAILED;
	}
	SendRes = SendBuffer(
		(const char *)(Str),
		(int)(TotalStringSizeInBytes),
		sd);

	return SendRes;
}
/**
 * Accepts:
 * -------
 * ReceiveBuffer() uses a socket to receive a buffer.
 * OutputBuffer - pointer to a buffer into which data will be written
 * OutputBufferSize - size in bytes of Output Buffer
 * BytesReceivedPtr - output parameter. if function returns TRNS_SUCCEEDED, then this
 *					  will point at an int containing the number of bytes received.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			printf("recv() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}
/**
 * ReceiveString() uses a socket to receive a string, and stores it in dynamic memory.
 *
 * Accepts:
 * -------
 * OutputStrPtr - a pointer to a char-pointer that is initialized to NULL, as in:
 *
 *		char *Buffer = NULL;
 *		ReceiveString( &Buffer, ___ );
 *
 * a dynamically allocated string will be created, and (*OutputStrPtr) will point to it.
 *
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */

TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd)
{
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;
	FD_SET ReadSet;
	int retval = -1;

	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
		return TRNS_FAILED;
	}
	FD_ZERO(&ReadSet);
	FD_SET(sd, &ReadSet);
	retval = select(0, &ReadSet, NULL, NULL, &time_wait);
	if (retval == 0)
	{
		printf("Connection Timed-out\n");
		return TRNS_FAILED;
	}
	if (retval == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return TRNS_FAILED;
	}
	RecvRes = ReceiveBuffer(
		(char *)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), 
		sd);

	if (RecvRes != TRNS_SUCCEEDED) return RecvRes;

	StrBuffer = (char*)malloc(TotalStringSizeInBytes * sizeof(char));
	if (StrBuffer == NULL)
	{
		printf("Receiving string memory allocation error.\n");
		return TRNS_FAILED;
	}
	FD_ZERO(&ReadSet);
	FD_SET(sd, &ReadSet);
	retval = -1;
	retval = select(0, &ReadSet, NULL, NULL, &time_wait);
	if (retval == 0)
	{
		printf("Connection Timed-out\n");
		return TRNS_FAILED;
	}
	if (retval == SOCKET_ERROR)
	{
		printf("select() failed, error %d\n", WSAGetLastError());
		return TRNS_FAILED;
	}
	RecvRes = ReceiveBuffer(
		(char *)(StrBuffer),
		(int)(TotalStringSizeInBytes),
		sd);

	if (RecvRes == TRNS_SUCCEEDED)
	{
		*OutputStrPtr = StrBuffer;
	}
	else
	{
		free(StrBuffer);
	}

	return RecvRes;
}

/*
•Description –This function compare between two massages and return if they are equel with no consideration of '\n' or '\0'
• Parameters – char *message, char *type - the two strings that is compared
• Returns – 0 if equal, another number if not
*/
int MessageType(char *message, char *type)
{
	char tmp[27];
	int i = 0;
	while( message[i] != '\n' && message[i] != ':')
	{
		tmp[i] = message[i];
		i++;
	}
	tmp[i] = '\0';
	return strcmp(tmp, type);
}

/*
•Description –This function extracts the information between the charecters ':' and '\n" and puts it in str info
• Parameters – char *message, char *info
• Returns – void
*/
void MessageInfo(char *message, char *info)
{
	int i = 1;
	int j = 0;
	while ( message[i-1] != ':')
		i++;
	while (message[i] != '\n')
	{
		info[j] = message[i];
		j++;
		i++;
	}
	info[j] = '\0';
	return;
}

/*
•Description –This function prints a massage according to the error that accured
• Parameters – TransferResult_t RecvRes, char* type- types of error "send" or "recieved"
• Returns – 1 if success, 0 else
*/
int TransferResult(TransferResult_t RecvRes, char* type)
{
	if (RecvRes != TRNS_SUCCEEDED)
	{
		if (RecvRes == TRNS_DISCONNECTED)
			printf("Connection closed while reading.\n");
		else if (type == "Send")
			printf("Service socket error while writing.\n");
		else
			printf("Service socket error while reading.\n");
		return 1;
	}
	return 0;
}
/*
•Description –This function gets characters until '\n' or 'EOF' is entered. 
• Parameters – void
• Returns –void
*/
void clean_stdin(void)
{
	int c;
	do {
		c = getchar();
	} while (c != '\n' && c != EOF);
}

/*
•Description –This function gets characters from file imtil it arrives to '\n' or 'EOF'.
• Parameters – FILE* fp- the file pointer we read in
• Returns –void
*/
void skip_line(FILE* fp)
{
	int c;
	do {
		c = fgetc(fp);
	} while (c != '\n' && c != EOF);
}
/*
•Description –This function builds the request the server needs to accept in the correct format from the string inputs it gets
• Parameters – char* dest- the request is build here , char* info, char* request information for the request
• Returns –void
*/
void BuildRequest(char* dest, char* info, char* request)
{
	int i = 0;
	int j = 0;
	while (request[i] != '\0')
	{
		dest[i] = request[i];
		i++;
	}
	j = 0;
	while (info[j] != '\0')
	{
		dest[i] = info[j];
		i++;
		j++;
	}
	dest[i] = '\n';
	dest[i + 1] = '\0';
}