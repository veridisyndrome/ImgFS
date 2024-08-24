/*
 * @file imgfs_server_services.c
 * @brief ImgFS server part, bridge between HTTP server layer and ImgFS library
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // uint16_t

#include "error.h"
#include "util.h" // atouint16
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"

// Main in-memory structure for imgFS
static struct imgfs_file fs_file;
static uint16_t server_port;

#define URI_ROOT "/imgfs"

/********************************************************************//**
 * Startup function. Create imgFS file and load in-memory structure.
 * Pass the imgFS file name as argv[1] and optionnaly port number as argv[2]
 ********************************************************************** */
int server_startup (int argc, char **argv)
{
    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    int ret = ERR_NONE;

    ret = do_open(argv[1], "rb", &fs_file);
    if (ret != ERR_NONE) {
        return ret;
    }
    print_header(&fs_file.header);

    if (argv[2] != NULL) {
        server_port = atouint16(argv[2]);
    }

    if (!server_port) {
        server_port = DEFAULT_LISTENING_PORT;
    }

    ret = http_init(server_port, handle_http_message);
    if (ret < ERR_NONE) {
        do_close(&fs_file);
        return ret;
    }

    printf("ImgFS server started on http://localhost: %u\n", server_port);

    return ERR_NONE;


}

/********************************************************************//**
 * Shutdown function. Free the structures and close the file.
 ********************************************************************** */
void server_shutdown (void)
{
    fprintf(stderr, "\nShutting down...\n");
    http_close();
    do_close(&fs_file);
}

/**********************************************************************
 * Sends error message.
 ********************************************************************** */
static int reply_error_msg(int connection, int error)
{
#define ERR_MSG_SIZE 256
    char err_msg[ERR_MSG_SIZE]; // enough for any reasonable err_msg
    if (snprintf(err_msg, ERR_MSG_SIZE, "Error: %s\n", ERR_MSG(error)) < 0) {
        fprintf(stderr, "reply_error_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "500 Internal Server Error", "",
                      err_msg, strlen(err_msg));
}

/**********************************************************************
 * Sends 302 OK message.
 ********************************************************************** */
static int reply_302_msg(int connection)
{
    char location[ERR_MSG_SIZE];
    if (snprintf(location, ERR_MSG_SIZE, "Location: http://localhost:%d/" BASE_FILE HTTP_LINE_DELIM,
                 server_port) < 0) {
        fprintf(stderr, "reply_302_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "302 Found", location, "", 0);
}

/**********************************************************************
 * Simple handling of http message.
 ********************************************************************** */
int handle_list_call(struct http_message msg, int connection)
{
    char **json;
    int ret = do_list(&fs_file, JSON, json);
    if (ret < 0) {
        return reply_error_msg(connection, ret);
    }

    ret = http_reply(connection, "200 OK", "Content-Type: application/json" HTTP_LINE_DELIM,
                     *json, strlen(*json));
    free(*json);
    json = NULL;
    return ret;
}

int handle_read_call(struct http_message msg, int connection)
{
    char* out = calloc(10, sizeof(char));
    if (out == NULL) {
        return reply_error_msg(connection, ERR_OUT_OF_MEMORY);
    }

    int ret = http_get_var(&msg.uri, "res", out, 9);
    if (ret <= 0) {
        free(out);
        out = NULL;
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    char* img_id = calloc(MAX_IMG_ID + 1, sizeof(char));
    if (img_id == NULL) {
        free(out);
        out = NULL;
        return reply_error_msg(connection, ERR_OUT_OF_MEMORY);
    }

    ret = http_get_var(&msg.uri, "img_id", img_id, MAX_IMG_ID);
    if (ret <= 0) {
        free(out);
        free(img_id);
        img_id = NULL;
        out = NULL;
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    uint32_t image_size = 0;
    char* buffer;

    int res = resolution_atoi(out);
    if (res == -1) {
        free(out);
        free(img_id);
        img_id = NULL;
        out = NULL;
        return reply_error_msg(connection, ERR_RESOLUTIONS);
    }

    ret = do_read(img_id, res, &buffer, &image_size, &fs_file);
    if (ret != ERR_NONE) {
        free(out);
        free(img_id);
        img_id = NULL;
        out = NULL;
        return reply_error_msg(connection, ret);
    }
    ret = http_reply(connection, "200 OK", "Content-Type: image/jpeg" HTTP_LINE_DELIM,
                     buffer, image_size);

    free(out);
    free(img_id);
    free(buffer);
    out = NULL;
    img_id = NULL;
    buffer = NULL;
    return ret;
}

int handle_delete_call(struct http_message msg, int connection)
{
    char* img_id = calloc(MAX_IMG_ID + 1, sizeof(char));
    if (img_id == NULL) {
        return reply_error_msg(connection, ERR_OUT_OF_MEMORY);
    }

    int ret = http_get_var(&msg.uri, "img_id", img_id, MAX_IMG_ID);
    if (ret <= 0) {
        free(img_id);
        img_id = NULL;
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }


    ret = do_delete(img_id, &fs_file);
    if (ret != ERR_NONE) {
        free(img_id);
        img_id = NULL;
        return reply_error_msg(connection, ret);
    }

    free(img_id);
    img_id = NULL;
    return reply_302_msg(connection);
}

int handle_insert_call(int connection)
{
    return reply_302_msg(connection);
}

int handle_http_message(struct http_message* msg, int connection)
{
    M_REQUIRE_NON_NULL(msg);
    debug_printf("handle_http_message() on connection %d. URI: %.*s\n",
                 connection,
                 (int) msg->uri.len, msg->uri.val);

    if (http_match_verb(&msg->uri, "/") || http_match_uri(msg, "/index.html")) {
        return http_serve_file(connection, BASE_FILE);
    }

    if (http_match_uri(msg, URI_ROOT "/list")) {
        return handle_list_call(*msg, connection);
    } else if (http_match_uri(msg, URI_ROOT "/insert") && http_match_verb(&msg->method, "POST")) {
        return handle_insert_call(connection);
    } else if (http_match_uri(msg, URI_ROOT "/read")) {
        return handle_read_call(*msg, connection);
    } else if (http_match_uri(msg, URI_ROOT "/delete")) {
        return handle_delete_call(*msg, connection);
    } else {
        return reply_error_msg(connection, ERR_INVALID_COMMAND);
    }
}
