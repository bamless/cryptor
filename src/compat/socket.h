#ifndef SOCKET_H
#define SOCKET_H

//Unix sockets
#ifdef __unix
#include <sys/types.h>  /*not required on linux but required on other unix systems*/
#include <sys/socket.h>
#include <unistd.h>

typedef int Socket;

#define socket_startup()
#define socket_cleanup()
#define socket_close(sock) close(sock)
#define is_socket_valid(sock) (sock >= 0)

//Windows sockets
#elif _WIN32
#include <winsock2.h>

typedef SOCKET Socket;

#define socket_startup()  do { \
        WSADATA data; \
        if(WSAStartup(MAKEWORD(2,2), &data)) { \
            perr_sock("Error: socket_startup"); \
            exit(1); \
        } \
    } while(0)
#define socket_cleanup() do { \
        if(WSACleanup()) { \
            perr_sock("Error: socket_cleanup"); \
            exit(1); \
        } \
    } while(0)
#define socket_close(sock) closesocket(sock)
#define is_socket_valid(sock) (sock != INVALID_SOCKET)
#endif

#endif
