#ifndef SOCKET_H
#define SOCKET_H

#define PORT_MAX 65535
#define PORT_MIN 1

/*
 * Header only compatibility module for sockets.
 */

//Unix sockets
#ifdef __unix
#include <sys/types.h>  /*not required on linux but required on other unix systems*/
#include <sys/socket.h>
#include <arpa/inet.h>
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
typedef int socklen_t; // needed in roder to use accept in a portable way (avoids compiler warnings)

#define MSG_NOSIGNAL 0 //option needed to stop the soket from sending sigpipes on linux

#define socket_startup()  do { \
        WSADATA data; \
        if(WSAStartup(MAKEWORD(2,2), &data)) { \
            perr_sock("Error: socket startup"); \
            exit(1); \
        } \
    } while(0)
#define socket_cleanup() do { \
        if(WSACleanup()) { \
            perr_sock("Error: socket cleanup"); \
            exit(1); \
        } \
    } while(0)
#define socket_close(sock) closesocket(sock)
#define is_socket_valid(sock) (sock != INVALID_SOCKET)
#endif

#endif
