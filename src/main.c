#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "server.h"
#include "worker_process.h"
#include "thread_pool.h"
#include "file_cache.h"
#include "performance_log.h"

#define NUM_WORKERS 2
#define NUM_THREADS_PER_WORKER 4

bool g_verbose = false; // verbose mode

file_cache_t g_file_cache;   // Cache globale
bool g_enable_zerocopy = false; // Flag globale (attenzione ai thread, ma qui va bene per demo)

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--zerocopy") == 0 || strcmp(argv[i], "-z") == 0) {
            // enable zerocopy mode
            g_enable_zerocopy = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            // enable verbose mode
            g_verbose = true;
        } else {
            int tmp = atoi(argv[i]);
            if (tmp > 0) {
                port = tmp;
            }
        }
    }

    // Inizializza la cache
    file_cache_init(&g_file_cache);

    // Inizializza performance log
    performance_log_init("performance.log");

    int listen_fd = create_listen_socket(port);
    if (listen_fd < 0) {
        fprintf(stderr, "Impossibile creare il socket in ascolto.\n");
        exit(EXIT_FAILURE);
    }
    if (g_verbose) {
        printf("[main] Server in ascolto sulla porta %d\n", port);
    }

    // Creiamo i processi worker
    for (int i = 0; i < NUM_WORKERS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Codice del processo figlio (worker)
            worker_process_t worker;
            worker.listen_fd = listen_fd;

            thread_pool_t pool;
            thread_pool_init(&pool, NUM_THREADS_PER_WORKER);
            worker.thread_pool = &pool;

            run_worker_process(&worker);

            thread_pool_destroy(&pool);
            exit(EXIT_SUCCESS);
        }
    }

    // Processo master: attende i worker
    while (1) {
        int status;
        pid_t wpid = wait(&status);
        if (wpid < 0) {
            break;
        }
        printf("Worker process %d terminato con status %d\n", wpid, status);
    }

    close(listen_fd);

    // Chiudiamo log
    performance_log_close();

    return 0;
}
