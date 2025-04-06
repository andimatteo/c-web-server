#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "request_parser.h"

/**
 * @brief Elabora la richiesta HTTP e invia una risposta al client_fd.
 *
 * @param client_fd file descriptor del client.
 * @param parser puntatore alla struttura con i campi del request parser.
 */
void handle_http_request(int client_fd, http_request_parser_t *parser);

#endif // HTTP_RESPONSE_H

