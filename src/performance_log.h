#ifndef PERFORMANCE_LOG_H
#define PERFORMANCE_LOG_H

#include <stddef.h>
#include <time.h>

/**
 * @brief Inizializza il log delle performance (ad es. apre file).
 */
void performance_log_init(const char *filename);

/**
 * @brief Registra un’operazione di invio file con dimensione e tempo impiegato.
 */
void performance_log_record(const char *path, size_t size, double time_sec);

/**
 * @brief Chiude il log file
 */
void performance_log_close();

#endif // PERFORMANCE_LOG_H
