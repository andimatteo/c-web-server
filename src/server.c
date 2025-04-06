#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @brief Imposta un socket in modalità non bloccante.
 *
 * @param fd file descriptor del socket.
 * @return 0 se ok, -1 se errore.
 */
static int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }

    return 0;
}

int create_listen_socket(int port) {
    int listen_fd;
    struct sockaddr_in server_addr;

    // Creazione del socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return -1;
    }

    // Opzione per riutilizzare l'indirizzo
    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind del socket
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    // Listen
    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    // Imposta non bloccante
    if (set_non_blocking(listen_fd) < 0) {
        close(listen_fd);
        return -1;
    }

    printf("Server in ascolto sulla porta %d\n", port);
    return listen_fd;
}

