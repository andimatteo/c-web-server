#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define DEFAULT_PORT 8080
#define BACKLOG 128
#define MAX_EVENTS 64

/**
 * @brief Crea e configura un socket in ascolto su una determinata porta.
 *
 * @param port la porta su cui mettersi in ascolto.
 * @return il file descriptor del socket in ascolto, oppure -1 in caso di errore.
 */
int create_listen_socket(int port);

#endif // SERVER_H

