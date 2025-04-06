#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <stddef.h>
#include <time.h>

typedef struct {
    char *content;
    size_t size;
    time_t last_modified;
} file_cache_entry_t;

typedef struct {
    char path[512];
    file_cache_entry_t entry;
} file_cache_slot_t;

#define FILE_CACHE_MAX_SLOTS 64

/**
 * @brief Struttura base di un file cache
 */
typedef struct {
    file_cache_slot_t slots[FILE_CACHE_MAX_SLOTS];
} file_cache_t;

/**
 * @brief Inizializza la cache.
 */
void file_cache_init(file_cache_t *cache);

/**
 * @brief Recupera il contenuto dalla cache. Se presente, restituisce puntatore a entry, altrimenti NULL.
 */
file_cache_entry_t* file_cache_get(file_cache_t *cache, const char *path);

/**
 * @brief Inserisce un file nella cache (path + contenuto).
 */
void file_cache_put(file_cache_t *cache, const char *path, const char *content, size_t size, time_t last_modified);

#endif // FILE_CACHE_H
