#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "util.h" // atouint16
#include "http_prot.h"

/**
 * @brief Extracts first substring before a delimiter
 *
 * @param message (const char*): Given message
 * @param delimiter (const char*): Given delimiter
 * @param output (struct http_string*): Given pointer to a http_string struct to write the value prior the delimiter
 * @return (const char*): Pointer after the first delimiter if there's a success, otherwise NULL
 */
static const char* get_next_token(const char* message, const char* delimiter, struct http_string* output)
{
    if (message == NULL || delimiter == NULL) {
        return NULL;
    }

    const char* ret = strstr(message, delimiter);
    if (ret == NULL) {
        return NULL;
    }

    size_t out_len = ret - message;

    if(output != NULL) {
        // http string is non-null terminated
        output->val = message;
        output->len = out_len;
    }

    ret += strlen(delimiter);

    return ret;
}


/**
 * @brief Fills all headers key-value pairs in an array
 *
 * @param header_start (const char*): Given header
 * @param output (struct http_message*): Given pointer to a http_message struct to write the header
 * @return (const char*): Pointer after header if there's a success, otherwise NULL
 */
static const char* http_parse_headers(const char* header_start, struct http_message* output)
{
    if (header_start == NULL || output == NULL) return NULL;

    size_t i = 0;

    do {
        // Extract key
        header_start = get_next_token(header_start, HTTP_HDR_KV_DELIM, &output->headers[i].key);
        if (header_start == NULL) return NULL;

        // Extract value
        header_start = get_next_token(header_start, HTTP_LINE_DELIM, &output->headers[i].value);
        if (header_start == NULL) return NULL;

        i++;
    } while((int)strncmp(header_start, HTTP_LINE_DELIM, strlen(HTTP_LINE_DELIM)));

    // Ensure we didn't reach MAX_HEADERS
    if (i == MAX_HEADERS) {
        return NULL;
    }

    // Move past the line delimiter
    header_start += strlen(HTTP_HDR_KV_DELIM);
    output->num_headers = i;

    return header_start;
}

/**
 * @brief Checks whether the `message` URI starts with the provided `target_uri`.
 *
 * @param message (http_message*): Given pointer to a http_message struct
 * @param target_uri (const char*): Given pointer to a  target uri found in the message
 * @return (int): 1 if it does, 0 if it does not.
 */
int http_match_uri(const struct http_message* message, const char* target_uri)
{
    M_REQUIRE_NON_NULL(message);
    M_REQUIRE_NON_NULL(target_uri);

    struct http_string uri = message->uri;
    if (uri.len < strlen(target_uri)) {
        return 0;
    }
    if (!strncmp(uri.val, target_uri, strlen(target_uri))) {
        printf(" \"%s\" match uri \"%.*s\"\n", target_uri, (int) uri.len, uri.val);
        return 1;
    } else {
        printf(" \"%s\" does not match uri \"%.*s\"\n", target_uri, (int) uri.len, uri.val);
        return 0;
    }

}

/**
 * @brief Checks whether the `method` with 'verb'
 *
 * @param method (http_string*): Given pointer to a http_string struct
 * @param verb (const char*): Given pointer to a verb found in the message
 * @return (int): 1 if it does, 0 if it does not.
 */
int http_match_verb(const struct http_string* method, const char* verb)
{
    if (method == NULL || verb == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if(method->len == strlen(verb) &&!strncmp(verb, method->val, method->len)) {
        printf("{val = \"%.*s\", len = %d} match verb \"%s\"\n",
               (int) method->len, method->val, (int) method->len, verb);
        return 1;
    } else {
        printf("{val = \"%.*s\", len = %d} does not match verb \"%s\"\n",
               (int) method->len, method->val, (int) method->len, verb);
        return 0;
    }
}

/**
 * @brief Writes the value of parameter `name` from URL in message to buffer out.
 *
 * @param url (http_string*): Given pointer to a http_string struct of the 'url'
 * @param name (const char*): Given name to find in the url
 * @param out (char*): Given pointer to the found value
 * @param out_len (size_t): Given length of the found value
 * @return (int): Length of name if it is found, 0 if it does not or error code
 */
int http_get_var(const struct http_string* url, const char* name, char* out, size_t out_len)

{
    M_REQUIRE_NON_NULL(url);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(out);
    if (out_len <= 0 ) {
        return ERR_INVALID_ARGUMENT;
    }

    char* param = calloc(strlen(name)+2, sizeof(char));
    if (param == NULL) return ERR_OUT_OF_MEMORY;

    strcpy(param, name);
    strcat(param, "=");

    char* name_found = NULL;
    // we look for the parameter name
    if ((name_found = strnstr(url->val, param, url->len)) == NULL) {
        free(param);
        param = NULL;
        return 0;
    }
    name_found += strlen(param);

    size_t remaining_size = url->len - (name_found - url->val);


    // we look if there is an ampersand after the string
    char* end = NULL;
    size_t name_size;
    if ((end = strnstr(name_found, "&", remaining_size)) == NULL) {
        name_size = remaining_size;
        if (name_size > out_len) {
            free(param);
            param = NULL;
            return ERR_RUNTIME;
        }
    } else {
        name_size = end - name_found;
        if (name_size > out_len) {
            free(param);
            param = NULL;
            return ERR_RUNTIME;
        }
    }

    strncpy(out, name_found, name_size);

    free(param);
    param = NULL;
    return (int) name_size;
}

/**
 * @brief Find Content Length in headers array
 *
 * @param verb (const char*): Given pointer to a verb found in the message
 * @return (int): Content Length if it is in the array, 0 if it is not or an error
 */
int http_content_len(struct http_message *out)
{
    M_REQUIRE_NON_NULL(out);

    int ret = 0;
    for (size_t i = 0; i < out->num_headers; ++i) {
        if (!strncmp(out->headers[i].key.val, "Content-Length", out->headers[i].key.len)) {
            return atoi(out->headers[i].value.val);
        }
    }
    return ret;
}

/**
 * @brief Accepts a potentially partial TCP stream and parses an HTTP message.
 *
 * Assumes that all characters of stream that are not filled by reading are set to 0.
 *
 * Places the complete HTTP message in out.
 * Also writes the content of header "Content Length" to content_len upon parsing the header in the stream.
 * content_len can be used by the caller to allocate memory to receive the whole HTTP message.
 *
 * Returns:
 *  a negative int if there was an error
 *  0 if the message has not been received completely (partial treatment)
 *  1 if the message was fully received and parsed
 */
int http_parse_message(const char *stream, size_t bytes_received, struct http_message *out, int *content_len)
{
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(out);
    M_REQUIRE_NON_NULL(content_len);

    // Check if headers are completely received
    if ((strnstr(stream, HTTP_HDR_END_DELIM, bytes_received)) == NULL) {
        return 0;
    }

    // Parse the request line
#define TOKEN_DELIMITER " "
    const char* token = get_next_token(stream, TOKEN_DELIMITER, &out->method);
    if (token == NULL) {
        return ERR_IO;
    }
    token = get_next_token(token, TOKEN_DELIMITER, &out->uri);
    if (token == NULL) {
        return ERR_IO;
    }

    // we check that the third token is valid
#define HTTP_TOKEN "HTTP/1.1"
    struct http_string http_check = {0};
    if (((token = get_next_token(token, HTTP_LINE_DELIM, &http_check)) == NULL) &&
        !strncpy((char *) http_check.val, HTTP_TOKEN, http_check.len)) {
        return ERR_IO;
    }

    // Parse headers
    const char* body = http_parse_headers(token, out);

    // Extract Content Length and put it in buffer
    *content_len = http_content_len(out);

    if (*content_len != 0) {
        size_t remaining_bytes = bytes_received - (body - stream);

        if (*content_len > remaining_bytes) {
            return 0;
        }

        out->body.val = body;
        out->body.len = *content_len;
    }

    return 1;
}
