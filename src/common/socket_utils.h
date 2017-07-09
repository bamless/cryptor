#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include "socket.h"

/*
 * Initializes a socket listening on port 'port'
 * @arg port the port on which the socket will be listening
 */
Socket init_server_socket(u_short port);
/*
 * Connects the socket to the server at 'adrr' over port 'port'
 * @arg addr the ip address of the host you want to connect to. This address should be already in network bytes order
 * @arg port the port over you want to connect. This should be already in network bytes order
 */
Socket connect_socket(unsigned long addr, u_short port);

#endif
