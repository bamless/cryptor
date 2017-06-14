#ifndef SOCKET_H
#define SOCKET_H

//Unix sockets
#ifdef __unix
#include <sys/types.h>  /*not required on linux but required on other unix systems*/
#include <sys/socket.h>
#include <unistd.h>

typedef int Socket;

#define socket_startup() 0
#define socket_cleanup() 0
#define socket_close(sock) close(sock)
#define is_socket_valid(sock) (sock >= 0)

//Windows sockets
#elif _WIN32
#include <winsock2.h>

typedef SOCKET Socket;

#define socket_startup()  WSAStartup(MAKEWORD(2,2), NULL)
#define socket_cleanup() WSACleanup()
#define socket_close(sock) closesocket(sock)
#define is_socket_valid(sock) (sock != INVALID_SOCKET)
#endif

#endif
