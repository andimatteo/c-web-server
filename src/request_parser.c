#include "request_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#define REQUEST_BUFFER_SIZE 16384  // 16 KB di default

static void trim(char *str) {
    // Rimuove spazi iniziali e finali
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start)+1);
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

void init_http_request_parser(http_request_parser_t *parser) {
    memset(parser->method, 0, sizeof(parser->method));
    memset(parser->path, 0, sizeof(parser->path));
    memset(parser->version, 0, sizeof(parser->version));

    parser->header_count = 0;
    for (int i = 0; i < MAX_HEADER_COUNT; i++) {
        memset(parser->headers[i].name, 0, sizeof(parser->headers[i].name));
        memset(parser->headers[i].value, 0, sizeof(parser->headers[i].value));
    }

    parser->body = NULL;
    parser->body_length = 0;
}

/**
 * @brief Legge dal socket finché non trova "\r\n\r\n" (fine header).
 *        Se Content-Length > 0, legge anche il body.
 */
void parse_http_request(int client_fd, http_request_parser_t *parser) {
    char buffer[REQUEST_BUFFER_SIZE];
    int bytes_read = 0;
    int total_read = 0;

    // Leggiamo dal socket in più passate, finché troviamo l'header completo
    // oppure finché esauriamo il buffer.
    // In un sistema reale, sarebbe necessario un approccio più robusto (cicli di recv + controlli).
    while ((bytes_read = read(client_fd, buffer + total_read,
                              REQUEST_BUFFER_SIZE - 1 - total_read)) > 0) {
        total_read += bytes_read;
        buffer[total_read] = '\0';

        // Cerchiamo la sequenza "\r\n\r\n"
        char *header_end = strstr(buffer, "\r\n\r\n");
        if (header_end) {
            // Abbiamo trovato la fine dell'header
            break;
        }
        // Se non troviamo nulla, continuiamo a leggere (o finisce buffer).
        if (total_read >= REQUEST_BUFFER_SIZE - 1) {
            break;
        }
    }

    // Se total_read == 0, potrebbe essere una connessione chiusa
    if (total_read <= 0) {
        return;
    }

    // A questo punto, buffer contiene almeno la request line + header (forse parziale).
    // Troviamo la fine dell'header
    char *header_end = strstr(buffer, "\r\n\r\n");
    int header_len = 0;
    if (header_end) {
        header_len = header_end - buffer + 4; // comprensivo di \r\n\r\n
    } else {
        // Non abbiamo trovato la fine dell'header.
        // Parser molto rudimentale, ci fermiamo.
        header_len = total_read;
    }

    // 1) Estraiamo la request line (prima riga)
    //    <METHOD> <PATH> <VERSION>\r\n
    char *line_start = buffer;
    char *line_end = strstr(line_start, "\r\n");
    if (!line_end) {
        // Strano, non troviamo la fine della request line
        return;
    }
    *line_end = '\0'; // terminazione stringa
    // Ora line_start contiene "GET /index.html HTTP/1.1"

    // Splittiamo con sscanf o simili
    // Attenzione ai limiti di lunghezza.
    sscanf(line_start, "%7s %1023s %15s", 
           parser->method, parser->path, parser->version);

    // 2) Processiamo gli header rimanenti
    char *headers_start = line_end + 2; // saltiamo \r\n

    // Adesso headers_start punta all'inizio del blocco header vero e proprio
    // Dividiamo riga per riga finché non arriviamo a "\r\n\r\n"
    int header_index = 0;
    char *cur = headers_start;
    while (1) {
        char *next_line = strstr(cur, "\r\n");
        if (!next_line) {
            break; 
        }
        if (next_line == cur) {
            // Riga vuota = fine header
            break;
        }

        *next_line = '\0'; // terminazione di questa riga
        // Ora "cur" contiene qualcosa tipo "Host: localhost"

        // Splittiamo in "name: value"
        char *colon_pos = strchr(cur, ':');
        if (colon_pos && header_index < MAX_HEADER_COUNT) {
            *colon_pos = '\0';
            colon_pos++;
            trim(cur);
            trim(colon_pos);

            strncpy(parser->headers[header_index].name, cur,
                    MAX_HEADER_NAME_LEN - 1);
            strncpy(parser->headers[header_index].value, colon_pos,
                    MAX_HEADER_VALUE_LEN - 1);
            header_index++;
        }

        cur = next_line + 2; // saltiamo \r\n
        if (header_index >= MAX_HEADER_COUNT) {
            break; // troppi header
        }
    }
    parser->header_count = header_index;

    // 3) Se c'è un body (Content-Length > 0), leggiamo
    // Calcoliamo se abbiamo già parte del body nel buffer
    int consumed = header_len; // i primi header_len byte erano l'header
    int body_in_buffer = total_read - consumed;

    // Cerchiamo Content-Length
    int content_length = 0;
    for (int i = 0; i < parser->header_count; i++) {
        if (strcasecmp(parser->headers[i].name, "Content-Length") == 0) {
            content_length = atoi(parser->headers[i].value);
            break;
        }
    }

    if (content_length > 0) {
        parser->body_length = content_length;
        parser->body = (char *)malloc(content_length + 1);
        if (!parser->body) {
            // out of memory
            return;
        }
        memset(parser->body, 0, content_length + 1);

        // Copiamo eventuale body già letto
        if (body_in_buffer > 0) {
            if (body_in_buffer > content_length) {
                body_in_buffer = content_length; 
            }
            memcpy(parser->body, buffer + consumed, body_in_buffer);
        }

        int remaining = content_length - body_in_buffer;
        // Se manca ancora del body, lo leggiamo
        while (remaining > 0) {
            bytes_read = read(client_fd, parser->body + (content_length - remaining), remaining);
            if (bytes_read <= 0) {
                break; // conn chiusa o errore
            }
            remaining -= bytes_read;
        }
    }
}

const char* get_header_value(const http_request_parser_t *parser, const char *header_name) {
    for (int i = 0; i < parser->header_count; i++) {
        if (strcasecmp(parser->headers[i].name, header_name) == 0) {
            return parser->headers[i].value;
        }
    }
    return NULL;
}
