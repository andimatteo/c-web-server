#ifndef WORKER_PROCESS_H
#define WORKER_PROCESS_H

#include "thread_pool.h"

/**
 * @brief Struttura che rappresenta un processo worker.
 *        Ogni processo ha un fd dell'event loop (epoll/kqueue) e un riferimento
 *        al socket in ascolto (listen_fd) e al thread pool.
 */
typedef struct {
    int event_loop_fd;
    int listen_fd;
    thread_pool_t *thread_pool;
} worker_process_t;

/**
 * @brief Funzione che esegue il loop principale di un processo worker:
 *        - Registra il socket di ascolto nell'event loop
 *        - Attende eventi (accetta nuove connessioni)
 *        - Inserisce i socket accettati nella coda del thread pool
 */
void run_worker_process(worker_process_t *worker);

#endif // WORKER_PROCESS_H
