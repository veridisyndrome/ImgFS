/*
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "http_net.h"
#include "socket_layer.h"
#include "error.h"
#include "http_prot.h"

static int passive_socket = -1;
static EventCallback cb;

#define MK_OUR_ERR(X) \
static int our_ ## X = X

MK_OUR_ERR(ERR_NONE);
MK_OUR_ERR(ERR_INVALID_ARGUMENT);
MK_OUR_ERR(ERR_OUT_OF_MEMORY);
MK_OUR_ERR(ERR_IO);

/*******************************************************************
 * Handle connection
 */
static void *handle_connection(void *arg)
{
    char* rcvbuf = calloc(MAX_HEADER_SIZE, sizeof(char));
    int content_len = 0;
    struct http_message msg = {0};
    int connection = *(int *) arg;

    int bytes_received = (int) tcp_read(connection, rcvbuf, MAX_HEADER_SIZE);
    if (bytes_received == -1) {
        free(rcvbuf);
        rcvbuf = NULL;
        return &our_ERR_IO;

    } else if (!bytes_received) {
        free(rcvbuf);
        rcvbuf = NULL;
        close(connection);
        return &our_ERR_NONE;
    }

    int ret = http_parse_message(rcvbuf, bytes_received, &msg, &content_len);

    if (ret == -1) {
        free(rcvbuf);
        rcvbuf = NULL;
        return &our_ERR_IO;
    }

    // the body was only partially filled, so we extend size of rcvbuf one time
    if (!ret) {
        char* new_rcvbuf = realloc(rcvbuf, MAX_HEADER_SIZE + content_len);
        if (!new_rcvbuf) {
            free(new_rcvbuf);
            new_rcvbuf = NULL;
            return &our_ERR_OUT_OF_MEMORY;
        }
        rcvbuf = new_rcvbuf;

        // we increase pointer to receive new bytes
        char* body = rcvbuf;
        body += MAX_HEADER_SIZE + content_len;

        ret = (int) tcp_read(connection, body, content_len);

        if(ret == -1) {
            free(rcvbuf);
            rcvbuf = NULL;
            return &our_ERR_IO;
        }

    }

    if (ret > 0) {
        cb(&msg, connection); //EventCallback
        free(rcvbuf);
        rcvbuf = NULL;
        content_len = 0;
        return &our_ERR_NONE;
    }

    // required arguments may be null
    return &our_ERR_INVALID_ARGUMENT;
}


/*******************************************************************
 * Init connection
 */
int http_init(uint16_t port, EventCallback callback)
{
    passive_socket = tcp_server_init(port);
    cb = callback;

    return passive_socket;
}

/*******************************************************************
 * Close connection
 */
void http_close(void)
{
    if (passive_socket > 0) {
        if (close(passive_socket) == -1)
            perror("close() in http_close()");
        else
            passive_socket = -1;
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void)
{
    int connection = tcp_accept(passive_socket);
    if (connection == -1) {
        return ERR_IO;
    }

    int ret = *(int *) handle_connection(&connection);

    return ret;
}

/*******************************************************************
 * Serve a file content over HTTP
 */
int http_serve_file(int connection, const char* filename)
{
    M_REQUIRE_NON_NULL(filename);

    // open file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to open file \"%s\"\n", filename);
        return http_reply(connection, "404 Not Found", "", "", 0);
    }

    // get its size
    fseek(file, 0, SEEK_END);
    const long pos = ftell(file);
    if (pos < 0) {
        fprintf(stderr, "http_serve_file(): Failed to tell file size of \"%s\"\n",
                filename);
        fclose(file);
        return ERR_IO;
    }
    rewind(file);
    const size_t file_size = (size_t) pos;

    // read file content
    char* const buffer = calloc(file_size + 1, 1);
    if (buffer == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to allocate memory to serve \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    const size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "http_serve_file(): Failed to read \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    // send the file
    const int  ret = http_reply(connection, HTTP_OK,
                                "Content-Type: text/html; charset=utf-8" HTTP_LINE_DELIM,
                                buffer, file_size);

    // garbage collecting
    fclose(file);
    free(buffer);
    return ret;
}

/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char *body, size_t body_len)
{
#define CONTENT_LENGTH_TXT "Content-Length: "
    size_t buf_len = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM)
                     + strlen(headers) +strlen(CONTENT_LENGTH_TXT)
                     + sizeof(size_t) + strlen(HTTP_HDR_END_DELIM) + 1;

    char* buf = calloc(buf_len, sizeof(char));
    if (buf == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

#define BUFFER_FORMAT "%s %s%s%s%s%zu%s"
    int ret = sprintf(buf, BUFFER_FORMAT,
                      HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM, headers,
                      CONTENT_LENGTH_TXT, body_len, HTTP_HDR_END_DELIM);
    if (ret < 0) {
        free(buf);
        buf = NULL;
        return ERR_IO;
    }

    if (!body_len) {
        tcp_send(connection, buf, buf_len);
    } else {
        char* new_buf = realloc(buf, buf_len + body_len + strlen(HTTP_LINE_DELIM) + 1);
        if (new_buf == NULL) {
            free(new_buf);
            new_buf = NULL;
            return ERR_OUT_OF_MEMORY;
        }
        buf = new_buf;
        strcat(buf, body);
        strcat(buf, HTTP_LINE_DELIM);

        tcp_send(connection, buf, strlen(buf));
    }

    free(buf);
    buf = NULL;
    return ERR_NONE;
}
