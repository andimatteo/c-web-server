#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#ifdef __APPLE__
    #define USE_KQUEUE
#else
    #define USE_EPOLL
#endif

/**
 * @brief Crea e restituisce un "event loop" (kqueue o epoll).
 * @return file descriptor dell'evento, o -1 in caso di errore.
 */
int create_event_loop();

/**
 * @brief Registra un file descriptor (per l'ascolto o la lettura) nell'event loop.
 * @param loop_fd file descriptor dell'event loop (kqueue o epoll).
 * @param fd file descriptor da registrare.
 * @return 0 se ok, -1 in caso di errore.
 */
int add_event(int loop_fd, int fd);

/**
 * @brief Attende eventi su loop_fd. Restituisce il numero di eventi pronti.
 * @param loop_fd file descriptor dell'event loop.
 * @param max_events numero massimo di eventi da gestire in un singolo giro.
 * @param timeout ms di timeout (o -1 per infinito).
 * @param fds_out array in cui verranno salvati i fd pronti in lettura.
 * @return numero di fds effettivamente pronti, oppure -1 in caso di errore.
 */
int wait_for_events(int loop_fd, int max_events, int timeout, int *fds_out);

#endif // EVENT_LOOP_H
