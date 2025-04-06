#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

/**
 * @brief Definizione di una struttura di lavoro (job).
 *        In questo caso, contiene semplicemente il file descriptor
 *        del client da servire.
 */
typedef struct job_t {
    int client_fd;
    struct job_t *next; // Linked list
} job_t;

/**
 * @brief Struttura thread pool. Contiene un array di thread, una coda di job,
 *        e le primitive di sincronizzazione.
 */
typedef struct {
    pthread_t *threads;
    int thread_count;

    job_t *job_queue_head;
    job_t *job_queue_tail;

    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;

    bool stop;
} thread_pool_t;

/**
 * @brief Inizializza il thread pool creando un certo numero di thread.
 *
 * @param pool puntatore al thread_pool_t.
 * @param num_threads numero di thread da creare.
 */
void thread_pool_init(thread_pool_t *pool, int num_threads);

/**
 * @brief Aggiunge un job (client_fd) alla coda del thread pool.
 *
 * @param pool puntatore al thread_pool_t.
 * @param client_fd file descriptor del client.
 */
void thread_pool_add_job(thread_pool_t *pool, int client_fd);

/**
 * @brief Chiude il thread pool e rilascia le risorse.
 *
 * @param pool puntatore al thread_pool_t.
 */
void thread_pool_destroy(thread_pool_t *pool);

#endif // THREAD_POOL_H

