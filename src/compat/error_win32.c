#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include<winsock2.h>

void perr(const char *msg) {
	//Get the error message, if any.
	DWORD errorMessageID = GetLastError();
	if(errorMessageID == 0)
		return;

	LPSTR buffer;
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       NULL,
                       errorMessageID,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR) &buffer,
                       0,
                       NULL)) {
		printf("Format message failed with %lu\n", GetLastError());
		return;
	}

	fprintf(stderr, "%s: %s", msg, buffer);
	HeapFree(GetProcessHeap(), 0, buffer);
}

void perr_sock(const char *msg) {
	//Get the error message, if any.
	DWORD errorMessageID = WSAGetLastError();
	if(errorMessageID == 0)
		return;

	LPSTR buffer;
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
					   NULL,
					   errorMessageID,
					   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   (LPSTR) &buffer,
					   0,
					   NULL)) {
		printf("Format message failed with %lu\n", GetLastError());
		return;
	}

	fprintf(stderr, "%s: %s", msg, buffer);
	HeapFree(GetProcessHeap(), 0, buffer);
}

void perr_exit(const char *msg) {
	perr(msg);
	exit(GetLastError());
}
