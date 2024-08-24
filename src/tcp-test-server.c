#include <string.h>
#include "socket_layer.h"
#include "error.h"
#include "util.h"
#include <stdlib.h>


int main(int argc, char* argv[])
{
    int ret;
    if (argc != 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    uint16_t valid_port = atouint16(argv[1]);
    if(!valid_port) {
        return ERR_INVALID_ARGUMENT;
    }
    printf("Server started on port %d\n", valid_port);

    int socket = tcp_server_init(valid_port);
    while(1) {
        printf("Waiting for a size...\n");
        int active_socket = tcp_accept(socket);

        char size[5];
        ret = (int) tcp_read(active_socket, size, 4);

        if (ret != -1) {
            int file_size;
            sscanf(size, "%d", &file_size);

            if (file_size < 1024) {
                tcp_send(active_socket, "Small file", strlen("Small file"));
            } else {
                tcp_send(active_socket, "Big file", strlen("Big file"));
            }

            char* file = calloc(file_size+1, sizeof(char));
            ret = (int) tcp_read(active_socket, file, file_size);

            if (ret != -1) {
                tcp_send(active_socket, "accepted", strlen("accepted"));

                printf("Received a size: %d --> accepted\n", file_size);
                printf("About to receive file of %d bytes\n", file_size);

                printf("Received a file:\n");
                printf("%s", file); putchar('\n');
            } else {
                tcp_send(active_socket, "refused", strlen("refused"));
            }
            free(file);
            file = NULL;
        } else {
            tcp_send(active_socket, "refused", strlen("refused"));
        }
    }
}