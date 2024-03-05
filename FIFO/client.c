//
// Created by wookie on 3/5/24.
//
#include "client.h"

#include <stdlib.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

int read_file_send_to_fifo(char* fifo_name, char* file_name)
{
    ssize_t count;
    char buffer[PIPE_BUF];
    int fd = open(file_name, O_RDONLY);
    if(fd < 0) ERR("open");

    int fifo = open(fifo_name, O_WRONLY);
    if(fifo < 0) ERR("open");

    while(read(fd, buffer, PIPE_BUF) > 0)
    {
        if((count = write(fifo, buffer, PIPE_BUF)) < 0) ERR("write");
    }

    if(close(fd) < 0) ERR("close");
    if(close(fifo) < 0) ERR("close");
}

int main(int argc, char** argv)
{
    if (argc != 3)  usage(argv[0]);
    char* fifo_name = argv[1];
    char* file_name = argv[2];

    read_file_send_to_fifo(fifo_name, file_name);
    return EXIT_SUCCESS;
}

