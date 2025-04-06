#include "worker_process.h"
#include "server.h"       // per MAX_EVENTS
#include "thread_pool.h"
#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>

extern bool g_verbose;

/**
 * @brief Accetta tutte le connessioni pendenti dal socket di ascolto
 *        e le passa al thread pool. Usa socket bloccanti per i client,
 *        così da poter gestire in modo semplice il keep-alive nei thread.
 */
static void accept_connections(worker_process_t *worker) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(worker->listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            // Nessuna connessione pendente o errore
            break;
        }

        // Log
        if (g_verbose) {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
            printf("[worker] Connessione accettata da %s:%d (fd=%d)\n",
                   ip_str, ntohs(client_addr.sin_port), client_fd);
        }

        // (RIMOSSO) Imposta non-bloccante? --> No, per keep-alive è più semplice bloccare.
        // int flags = fcntl(client_fd, F_GETFL, 0);
        // fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        // Passiamo la connessione (file descriptor) al thread pool
        thread_pool_add_job(worker->thread_pool, client_fd);
    }
}

/**
 * @brief Funzione del processo worker: crea un event loop (kqueue/epoll)
 *        ma lo usa solo per registrare il socket di ascolto, così da accorgersi
 *        di nuove connessioni in arrivo. I client vengono gestiti dal thread pool.
 */
void run_worker_process(worker_process_t *worker) {
    worker->event_loop_fd = create_event_loop();
    if (worker->event_loop_fd < 0) {
        perror("create_event_loop");
        exit(EXIT_FAILURE);
    }

    // Registriamo il socket di ascolto nell'event loop
    if (add_event(worker->event_loop_fd, worker->listen_fd) < 0) {
        close(worker->event_loop_fd);
        exit(EXIT_FAILURE);
    }

    int *active_fds = (int*)malloc(sizeof(int) * MAX_EVENTS);
    if (!active_fds) {
        perror("malloc active_fds");
        exit(EXIT_FAILURE);
    }

    // Loop principale di attesa eventi
    while (1) {
        int n = wait_for_events(worker->event_loop_fd, MAX_EVENTS, -1, active_fds);
        if (n < 0) {
            perror("wait_for_events");
            continue;
        }

        // Controlliamo gli fd "attivi"
        for (int i = 0; i < n; i++) {
            // Se l'fd è quello di ascolto, accettiamo le nuove connessioni
            if (active_fds[i] == worker->listen_fd) {
                accept_connections(worker);
            }
            // (Altri fd client, se volessimo gestire I/O via epoll/kqueue,
            //  ma qui li passiamo subito al thread pool.)
        }
    }

    free(active_fds);
    close(worker->event_loop_fd);
}
