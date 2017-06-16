#ifndef CRYPTORCLIENT_H
#define CRYPTORCLIENT_H

#include "socket.h"

int cryptor_send_command(Socket *sock, const char *cmd, unsigned int seed, const char *path);

#endif
