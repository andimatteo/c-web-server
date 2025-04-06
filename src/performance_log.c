#include "performance_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static FILE *g_perf_log_file = NULL;

void performance_log_init(const char *filename) {
    g_perf_log_file = fopen(filename, "a");
    if (!g_perf_log_file) {
        perror("fopen performance_log");
    }
}

void performance_log_record(const char *path, size_t size, double time_sec) {
    if (!g_perf_log_file) return;

    fprintf(g_perf_log_file, "FILE: %s SIZE: %zu TIME: %.4f sec\n", path, size, time_sec);
    fflush(g_perf_log_file);
}

void performance_log_close() {
    if (g_perf_log_file) {
        fclose(g_perf_log_file);
        g_perf_log_file = NULL;
    }
}
