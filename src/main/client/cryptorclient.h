#ifndef CRYPTORCLIENT_H
#define CRYPTORCLIENT_H

#include "socket.h"
#include "stringbuf.h"

/*
 * The actual implementation of the protocol
 */


/*
 * Sends a command (@see protocol.h) to the server represented by sock.
 * @arg server a socket connect()ed to a server running the protocol.
 * @arg cmd a protocol command @see protocol.h for more details
 * @arg seed seed for the ENCR and DECR command. It is ingored by other commands
 * @arg path file path for the ENCR and DECR command. It is ingored by other commands (Nullable)
 * @return the server response code @see protocol.h
 */
int cryptor_send_command(Socket server, const char *cmd, unsigned int seed, const char *path);
/*
 * Reads further output from the server and returns it in sb.
 * This function should be called only if the return code from a command is
 * RETMORE (i.e. 300).
 * @arg server a socket connect()ed to a server running the protocol.
 * @arg sb the string buffer that will hold the output
 */
void cryptor_read_more(Socket server, StringBuffer *sb);
/*
 * Prints further output from the server to stdout. This function should be called only if
 * the return code from a command is RETMORE (i.e. 300).
 * @arg server a socket connect()ed to a server running the protocol.
 */
void cryptor_print_more(Socket server);

#endif
