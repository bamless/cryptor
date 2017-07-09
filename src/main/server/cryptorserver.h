#ifndef CRYPTORSERVER_H
#define CRYPTORSERVER_H

#include "socket.h"

/*
 * Handles the connection with a client, it receives commands and respond to them
 * according to the protocol.
 * @arg client A socket that has been accept()ed and that is connected to a protocol client.
 */
void cryptor_handle_connection(Socket client);

#endif
