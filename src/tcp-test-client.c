#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "socket_layer.h"
#include "error.h"
#include "util.h"
#include "stdlib.h"

static int acknowledgement(int active_socket, FILE* fd);

int main(int argc, char* argv[])
{
    int ret;
    if (argc != 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    uint16_t valid_port = atouint16(argv[1]);
    if(!valid_port) {
        return ERR_INVALID_ARGUMENT;
    }
    int active_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (active_socket == -1) {
        perror("opening stream socket\n");
        return ERR_IO;
    }
    struct sockaddr_in sin = {AF_INET, htons(valid_port), htonl(INADDR_LOOPBACK)};

    ret = connect(active_socket, (struct sockaddr *) &sin, sizeof sin);
    if (ret == -1) {
        perror("connecting to active socket\n");
        return ERR_IO;
    }
    printf("Talking to %d\n", valid_port);

    FILE* fd = fopen(argv[2], "rb");
    if(fd == NULL) {
        close(active_socket);
        return ERR_IO;
    }

    // test if file has a size less than 2048
    fseek(fd, 0, SEEK_END);
    int file_size = (int) ftell(fd);
    printf("Sending size %d:\n", file_size);
    if (file_size >= 2048) {
        close(active_socket);
        fclose(fd);
        return ERR_INVALID_ARGUMENT;
    }

    // sending file size to server
    char size[5];
    sprintf(size, "%d", file_size);
    ret = (int) tcp_send(active_socket, size, 4);
    if (ret == -1) {
        close(active_socket);
        fclose(fd);
        return ERR_IO;
    }

    char* res = calloc(11, sizeof(char));
    if (res == NULL) {
        close(active_socket);
        fclose(fd);
        return ERR_OUT_OF_MEMORY;
    }

    ret = (int) tcp_read(active_socket, res, 11);
    if (ret == -1) {
        close(active_socket);
        fclose(fd);
        free(res);
        res =NULL;
        return ERR_IO;
    }


    if (!strcmp(res, "refused")) {
        close(active_socket);
        fclose(fd);
        free(res);
        res =NULL;

        printf("Refused\n");
        printf("Shutdown\n");
        return ERR_INVALID_ARGUMENT;
    }
    printf("Server responded: \"%s\"\n", res);
    free(res);
    res =NULL;


    // send file to the server
    fseek(fd, 0, SEEK_SET);
    char* file = calloc(1, file_size);
    if (file == NULL) {
        close(active_socket);
        fclose(fd);
        return ERR_OUT_OF_MEMORY;
    }

    ret = (int) fread(file, file_size, 1, fd);
    if (ret == -1) {
        close(active_socket);
        fclose(fd);
        free(file);
        file = NULL;
        return ERR_IO;
    }

    printf("Sending %s:\n", argv[2]);
    ret = (int) tcp_send(active_socket, file, file_size);
    free(file);
    if (ret == -1) {
        close(active_socket);
        fclose(fd);
    }

    return acknowledgement(active_socket, fd);
}


int acknowledgement(int active_socket, FILE* fd)
{
    char* buf = calloc(1, 10);
    if (buf == NULL) {
        close(active_socket);
        fclose(fd);
        return ERR_OUT_OF_MEMORY;
    }

    int ret = (int) tcp_read(active_socket, buf, 10);
    if (ret == -1) {
        close(active_socket);
        fclose(fd);
        free(buf);
        buf =NULL;
        return ERR_IO;
    }

    int ack = !strcmp(buf, "refused");

    free(buf);
    buf = NULL;
    if (ack) {
        close(active_socket);
        fclose(fd);

        printf("Refused\n");
        printf("Shutdown\n");
        return ERR_INVALID_ARGUMENT;
    } else {
        close(active_socket);
        fclose(fd);

        printf("Accepted\n");
        printf("Done\n");
        return ERR_NONE;
    }
}