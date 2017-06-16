#include "connhandler.h"
#include "socket.h"
#include "utilsCompat.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>

void handle_connection(void *socket, int id) {
    Socket client = *((Socket *) socket);
    free(socket);

    logf("Thread %d is handling connection\n", id);

    char cmd[4];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    logf("Received command %s\n", cmd);
    socket_close(client);
    logf("Thread %d done\n", id);
}
