#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <stdbool.h>

#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN 1024
#define MAX_VERSION_LEN 16
#define MAX_HEADER_COUNT 50
#define MAX_HEADER_NAME_LEN 64
#define MAX_HEADER_VALUE_LEN 1024

typedef struct {
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} http_header_t;

/**
 * @brief Struttura di parsing della richiesta HTTP.
 */
typedef struct {
    char method[MAX_METHOD_LEN];        // GET, POST, PUT, ecc.
    char path[MAX_PATH_LEN];            // /index.html
    char version[MAX_VERSION_LEN];      // HTTP/1.1, HTTP/1.0, ecc.

    http_header_t headers[MAX_HEADER_COUNT];
    int header_count;

    char *body;                         // Puntatore a un buffer con il body (se presente)
    int body_length;
} http_request_parser_t;

/**
 * @brief Inizializza il parser, azzerando i campi.
 */
void init_http_request_parser(http_request_parser_t *parser);

/**
 * @brief Legge dal socket e riempie la struttura parser con:
 *        - Request line (method, path, version)
 *        - Headers (fino a MAX_HEADER_COUNT)
 *        - Body (se Content-Length > 0)
 *
 * @param client_fd fd del socket
 * @param parser puntatore alla struttura parser
 */
void parse_http_request(int client_fd, http_request_parser_t *parser);

/**
 * @brief Recupera il valore di un header (es. "Host", "User-Agent").
 *        Restituisce puntatore alla stringa value, oppure NULL se non trovato.
 */
const char* get_header_value(const http_request_parser_t *parser, const char *header_name);

#endif // REQUEST_PARSER_H
