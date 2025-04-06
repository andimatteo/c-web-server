#include "file_cache.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void file_cache_init(file_cache_t *cache) {
    for (int i = 0; i < FILE_CACHE_MAX_SLOTS; i++) {
        cache->slots[i].path[0] = '\0';
        cache->slots[i].entry.content = NULL;
        cache->slots[i].entry.size = 0;
        cache->slots[i].entry.last_modified = 0;
    }
}

static int find_slot_index(file_cache_t *cache, const char *path) {
    // Semplice scan lineare (inefficiente ma semplice)
    for (int i = 0; i < FILE_CACHE_MAX_SLOTS; i++) {
        if (strcmp(cache->slots[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}

file_cache_entry_t* file_cache_get(file_cache_t *cache, const char *path) {
    int idx = find_slot_index(cache, path);
    if (idx < 0) {
        return NULL;
    }
    return &cache->slots[idx].entry;
}

void file_cache_put(file_cache_t *cache, const char *path, const char *content, size_t size, time_t last_modified) {
    // Se esiste già, sovrascriviamo
    int idx = find_slot_index(cache, path);

    if (idx < 0) {
        // Trova uno slot libero
        for (int i = 0; i < FILE_CACHE_MAX_SLOTS; i++) {
            if (cache->slots[i].path[0] == '\0') {
                idx = i;
                break;
            }
        }
    }

    if (idx < 0) {
        // Non abbiamo trovato slot: niente caching
        return;
    }

    strncpy(cache->slots[idx].path, path, sizeof(cache->slots[idx].path) - 1);
    if (cache->slots[idx].entry.content) {
        free(cache->slots[idx].entry.content);
        cache->slots[idx].entry.content = NULL;
    }
    cache->slots[idx].entry.content = (char*)malloc(size);
    if (!cache->slots[idx].entry.content) {
        return;
    }
    memcpy(cache->slots[idx].entry.content, content, size);
    cache->slots[idx].entry.size = size;
    cache->slots[idx].entry.last_modified = last_modified;
}
