#include "thread_pool.h"
#include "request_parser.h"
#include "http_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdbool.h>

extern bool g_verbose;

/**
 * @brief Controlla se la connessione deve restare aperta (keep-alive) o no
 *
 * Politica semplificata: 
 * - se "Connection: close" => chiude
 * - se "HTTP/1.0" senza header => chiude
 * - altrimenti (HTTP/1.1 default o "Connection: keep-alive") => keep-alive
 */
static bool should_keep_alive(const http_request_parser_t *parser) {
    // Cerchiamo l'header "Connection"
    const char *conn = NULL;
    for (int i = 0; i < parser->header_count; i++) {
        if (strcasecmp(parser->headers[i].name, "Connection") == 0) {
            conn = parser->headers[i].value;
            break;
        }
    }

    // Se c'è "Connection: close" => no keep-alive
    if (conn && strcasecmp(conn, "close") == 0) {
        return false;
    }

    // HTTP/1.0 di default non fa keep-alive se non lo dichiari esplicitamente
    if (strncmp(parser->version, "HTTP/1.0", 8) == 0) {
        if (conn && strcasecmp(conn, "keep-alive") == 0) {
            return true;
        }
        return false;
    }

    // HTTP/1.1 di default ha keep-alive, salvo "Connection: close"
    return true;
}

/**
 * @brief Gestisce una singola connessione client in un loop:
 *        parse => handle => decide se keep-alive => parse => ...
 */
static void handle_connection(int client_fd) {
    // Impostiamo un timeout (es. 5s) per non restare bloccati per sempre
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    while (1) {
        http_request_parser_t parser;
        init_http_request_parser(&parser);

        // Leggiamo e parsiamo la request
        parse_http_request(client_fd, &parser);

        // Se non abbiamo letto nulla (parser->method[0] == '\0'), 
        // significa conn chiusa o timeout => esci
        if (parser.method[0] == '\0') {
            if (g_verbose) {
                printf("[thread_pool] Nessuna request letta (fd=%d). Chiudo.\n", client_fd);
            }
            break;
        }

        // Genera risposta
        handle_http_request(client_fd, &parser);

        // Decide se rimanere aperti
        if (!should_keep_alive(&parser)) {
            if (g_verbose) {
                printf("[thread_pool] Chiusura post-richiesta su fd=%d (no keep-alive)\n", client_fd);
            }
            break;
        }

        // Se vogliamo restare aperti, passiamo al loop successivo per una nuova request
        if (g_verbose) {
            printf("[thread_pool] Resto in keep-alive su fd=%d\n", client_fd);
        }
    }

    // Chiudiamo definitivamente la connessione
    close(client_fd);
}

/**
 * @brief Funzione eseguita da ogni thread del pool:
 *        - Preleva un job (un client_fd) dalla coda
 *        - Gestisce la connessione con handle_connection (che può servire più request se keep-alive)
 *        - Ritorna in attesa di un nuovo job
 */
static void *thread_pool_worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);

        // Attende finché la coda è vuota e non è arrivato lo stop
        while (pool->job_queue_head == NULL && !pool->stop) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }

        if (pool->stop) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }

        // Estrae il job dalla coda
        job_t *job = pool->job_queue_head;
        if (job) {
            if (g_verbose) {
                printf("[thread_pool] Inizio gestione connessione su fd=%d\n", job->client_fd);
            }
            pool->job_queue_head = job->next;
            if (!pool->job_queue_head) {
                pool->job_queue_tail = NULL;
            }
        }
        pthread_mutex_unlock(&pool->queue_mutex);

        if (job) {
            // Gestiamo (potenzialmente più richieste) finché c'è keep-alive
            handle_connection(job->client_fd);

            // Libera la struttura job
            free(job);
        }
    }
    return NULL;
}

void thread_pool_init(thread_pool_t *pool, int num_threads) {
    pool->thread_count = num_threads;
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    pool->job_queue_head = NULL;
    pool->job_queue_tail = NULL;
    pool->stop = false;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool);
    }
}

void thread_pool_add_job(thread_pool_t *pool, int client_fd) {
    job_t *new_job = (job_t *)malloc(sizeof(job_t));
    new_job->client_fd = client_fd;
    new_job->next = NULL;

    pthread_mutex_lock(&pool->queue_mutex);

    if (pool->job_queue_tail == NULL) {
        pool->job_queue_head = new_job;
        pool->job_queue_tail = new_job;
    } else {
        pool->job_queue_tail->next = new_job;
        pool->job_queue_tail = new_job;
    }

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
}

void thread_pool_destroy(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    pool->stop = true;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Join threads
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    // Libera la coda residua
    while (pool->job_queue_head) {
        job_t *tmp = pool->job_queue_head;
        pool->job_queue_head = pool->job_queue_head->next;
        free(tmp);
    }

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
}
