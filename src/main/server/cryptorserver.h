#ifndef CRYPTORSERVER_H
#define CRYPTORSERVER_H

#include "socket.h"

/*
 * Initializes a socket listening on port 'port'
 * @arg port the port on which the socket will be listening
 */
Socket init_server_socket(u_short port);
/*
 * Handles the conction with a client, it receives commands and respond to them
 * according to the protocol.
 * @arg client A socket that has been accept()ed and that is connected to a protocol client.
 */
void cryptor_handle_connection(Socket client);

#endif
