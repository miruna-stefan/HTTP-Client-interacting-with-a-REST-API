#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"

char *compute_get_request(const char *host, const char *url, char *query_params,
                            char **cookies, int cookies_count, char *token)
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    DIE(message == NULL, "calloc failed");
    char *line = (char*) calloc(LINELEN, sizeof(char));
    DIE(line == NULL, "calloc failed");

    // Write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add cookies
    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");

        for (int i = 0; i < cookies_count - 1; i++) {
            strcat(line, cookies[i]);
            strcat(line, ";");
        }

        /* separate the instruction for the last cookie from the
        rest because it doesn't have a ";" at the end */
        strcat(line, cookies[cookies_count - 1]);
        compute_message(message,line);
    }

    // Add token
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Add final new line
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(const char *host, const char *url, const char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, char *token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    DIE(message == NULL, "calloc failed");
    char *line = (char *)calloc(LINELEN, sizeof(char));
    DIE(line == NULL, "calloc failed");
    char *body_data_buffer = (char *)calloc(LINELEN, sizeof(char));
    DIE(body_data_buffer == NULL, "calloc failed");

    // Write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Add the host    
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add content type
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Transfer in the body_data_buffer body_data[0]
    if (body_data != NULL) {
        for (int i = 0; i < body_data_fields_count - 1; i++) {
            strcat(body_data_buffer, body_data[i]);
            // Add & between fields
            strcat(body_data_buffer, "&");
        }
        /* separate the instruction for the last field from the
        rest because it doesn't have a "&" at the end */
        strcat(body_data_buffer, body_data[body_data_fields_count - 1]);
    }

    int body_data_size = strlen(body_data_buffer);

    // Add content length
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %d", body_data_size);
    compute_message(message, line);

    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");

        for (int i = 0; i < cookies_count - 1; i++) {
            strcat(line, cookies[i]);
            strcat(line, ";");
        }

        /* separate the instruction for the last cookie from the
        rest because it doesn't have a ";" at the end */
        strcat(line, cookies[cookies_count - 1]);
        compute_message(message,line);
    }

    // Add token
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Add new line at end of header
    compute_message(message, "");

    // Add the actual payload data
    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);
    compute_message(message, "");

    free(line);
    return message;
    return message;
}

char *compute_delete_request(const char *host, const char *url,
                            char **cookies, int cookies_count, char *token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    DIE(message == NULL, "calloc failed");
    char *line = (char *)calloc(LINELEN, sizeof(char));
    DIE(line == NULL, "calloc failed");

    // Write the method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Add the host    
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add cookies
    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");

        for (int i = 0; i < cookies_count - 1; i++) {
            strcat(line, cookies[i]);
            strcat(line, ";");
        }

        /* separate the instruction for the last cookie from the
        rest because it doesn't have a ";" at the end */
        strcat(line, cookies[cookies_count - 1]);
        compute_message(message,line);
    }

    // Add token
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Add new line at end of header
    compute_message(message, "");

    free(line);
    return message;

}