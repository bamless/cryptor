#ifndef CRYPTORCLIENT_H
#define CRYPTORCLIENT_H

#include "socket.h"

/*
 *The actual implementation of the protocol
 */

/*
 * Connects the socket to the server at 'adrr' over port 'port'
 * @arg addr the ip address of the host you want to connect to. This address should be already in network bytes order
 * @arg poer the port over you want to connect. This should be already in network bytes order
 */
Socket init_connection(unsigned long addr, u_short port);

/*
 * Sends a command (@see protocol.h) to the server represented by sock.
 * @arg sock a socket connect()ed to a server running the protocol.
 * @arg cmd a protocol command @see protocol.h for more details
 * @arg seed seed for the ENCR and DECR command. It is ingored by other commands
 * @arg path file path for the ENCR and DECR command. It is ingored by other commands (Nullable)
 * @return the server response code @see protocol.h
 */
int cryptor_send_command(Socket sock, const char *cmd, unsigned int seed, const char *path);

#endif
