#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "socket_layer.h"
#include "error.h"

int tcp_server_init(uint16_t port)
{
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        perror("opening stream socket");
        return ERR_IO;
    }

    struct sockaddr_in sin = {AF_INET, htons(port), htonl(INADDR_LOOPBACK)};

    if (bind(tcp_socket, (struct sockaddr *) &sin, sizeof sin) == -1) {
        perror("binding stream socket");
        close(tcp_socket);
        return ERR_IO;
    }

#define MAX_PENDING 100
    if (listen(tcp_socket, MAX_PENDING) == -1) {
        perror("listening stream message");
        close(tcp_socket);
        return ERR_IO;
    }

    return tcp_socket;
}

/**
 * @brief Blocking call that accepts a new TCP connection
 */
int tcp_accept(int passive_socket)
{
    return accept(passive_socket, NULL, NULL);
}


/**
 * @brief Blocking call that reads the active socket once and stores the output in buf
 */
ssize_t tcp_read(int active_socket, char* buf, size_t buflen)
{
    M_REQUIRE_NON_NULL(buf);
    return recv(active_socket, buf, buflen, 0);
}

ssize_t tcp_send(int active_socket, const char* response, size_t response_len)
{
    M_REQUIRE_NON_NULL(response);
    return send(active_socket, response, response_len, 0);
}
