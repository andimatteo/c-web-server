#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include "http_response.h"
#include "file_cache.h"
#include "performance_log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>




extern file_cache_t g_file_cache;   // definita altrove
extern bool g_enable_zerocopy;      // definito in main.c
extern bool g_verbose;              // definito in main.c

static void send_data(int client_fd, const char *data) {
    size_t len = strlen(data);
    write(client_fd, data, len);

    // log
    if (g_verbose) {
        printf("[response] Inviati %zu bytes a fd=%d\n", len, client_fd);
    }

}

static const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "text/plain";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0)  return "text/css";
    if (strcmp(ext, ".js") == 0)   return "application/javascript";
    if (strcmp(ext, ".png") == 0)  return "image/png";
    if (strcmp(ext, ".jpg") == 0)  return "image/jpeg";
    return "text/plain";
}

static void zero_copy_sendfile(int out_fd, int in_fd, size_t file_size) {
#ifdef __APPLE__
    // macOS sendfile
    off_t len = file_size;
    // macOS signature: sendfile(in_fd, out_fd, offset, len, hdtr, flags)
    // ma invertito: int sendfile(int fd, int s, off_t offset, off_t *len, struct sf_hdtr *hdtr, int flags);
    off_t offset = 0;
    sendfile(in_fd, out_fd, offset, &len, NULL, 0);
#else
    // Linux sendfile
    off_t offset = 0;
    ssize_t sent = sendfile(out_fd, in_fd, &offset, file_size);
    (void)sent; // ignoriamo il valore di ritorno per semplicità
#endif
}

/**
 * @brief Serve un file statico con supporto caching e zero-copy
 */
static void serve_file(int client_fd, const char *path) {
    char local_path[512];
    if (strcmp(path, "/") == 0) {
        snprintf(local_path, sizeof(local_path), "docs/index.html");
    } else {
        snprintf(local_path, sizeof(local_path), "docs%s", path);
    }

    // Inizia la misura del tempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Controllo in cache
    file_cache_entry_t *cached = file_cache_get(&g_file_cache, local_path);
    if (cached) {
        // Inviamo l’header
        send_data(client_fd, "HTTP/1.1 200 OK\r\n");
        char ctype[128];
        snprintf(ctype, sizeof(ctype), "Content-Type: %s\r\n", get_mime_type(local_path));
        send_data(client_fd, ctype);

        char clength[128];
        snprintf(clength, sizeof(clength), "Content-Length: %zu\r\n", cached->size);
        send_data(client_fd, clength);

        send_data(client_fd, "\r\n"); // fine header

        // Inviamo il contenuto (per semplicità qui con write, zero-copy da memoria non è banale)
        write(client_fd, cached->content, cached->size);

        // Log performance
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed = (end_time.tv_sec - start_time.tv_sec)
                         + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        performance_log_record(local_path, cached->size, elapsed);
        return;
    }

    // Se non in cache, apriamo il file
    int fd = open(local_path, O_RDONLY);
    if (fd < 0) {
        // 404
        send_data(client_fd, "HTTP/1.1 404 Not Found\r\n");
        send_data(client_fd, "Content-Type: text/plain\r\n\r\n");
        send_data(client_fd, "File not found.\r\n");
        return;
    }

    // Stat per dimensione e last-modified
    struct stat st;
    fstat(fd, &st);

    send_data(client_fd, "HTTP/1.1 200 OK\r\n");
    char content_type_header[128];
    snprintf(content_type_header, sizeof(content_type_header),
             "Content-Type: %s\r\n", get_mime_type(local_path));
    send_data(client_fd, content_type_header);

    char content_length_header[128];
    snprintf(content_length_header, sizeof(content_length_header),
         "Content-Length: %lld\r\n", (long long)st.st_size);
    send_data(client_fd, content_length_header);

    send_data(client_fd, "\r\n"); // fine header

    // Se abilitato zero-copy, usiamo sendfile
    if (g_enable_zerocopy) {
        zero_copy_sendfile(client_fd, fd, st.st_size);
    } else {
        // Fall-back a lettura e write manuale
        char file_buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes_read);
        }
    }

    // Memorizziamo in cache (attenzione alla memoria su file di grandi dimensioni!)
    lseek(fd, 0, SEEK_SET);
    char *content_buf = (char*)malloc(st.st_size);
    if (content_buf) {
        read(fd, content_buf, st.st_size);
        file_cache_put(&g_file_cache, local_path, content_buf, st.st_size, st.st_mtime);
        free(content_buf);
    }

    close(fd);

    // Log performance
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = (end_time.tv_sec - start_time.tv_sec)
                     + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    performance_log_record(local_path, st.st_size, elapsed);
}

void handle_http_request(int client_fd, http_request_parser_t *parser) {
    if (strcmp(parser->method, "GET") == 0) {
        serve_file(client_fd, parser->path);
    } else {
        send_data(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n");
        send_data(client_fd, "Content-Type: text/plain\r\n\r\n");
        send_data(client_fd, "Method Not Allowed\r\n");
    }
}
