#include "cryptorserver.h"
#include "socket.h"
#include "utilsCompat.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>

void cryptor_handle_connection(void *socket, int id) {
    Socket client = *((Socket *) socket);
    free(socket);

    logf("Thread %d is handling connection\n", id);

    char cmd[5];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    logf("Received command %s\n", cmd);

    send(client, "200", 3, 0);

    socket_close(client);
    logf("Thread %d done\n", id);
}
