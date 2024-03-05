//
// Created by wookie on 3/5/24.
//

/*
This is a client program written in C for inter-process communication using
FIFOs (named pipes). It reads data from a file and sends it to a FIFO.

The 'read_file_send_to_fifo' function reads data from a file and sends it to a
FIFO. It first opens the file and the FIFO. Then, it reads data from the file
into a buffer and writes the buffer to the FIFO. The process PID is added at
the front of each message. If the read data is less than the message size, the
remaining part of the message is filled with zeros. The function continues
reading and sending until there is no more data to read from the file.

The 'main' function is the entry point of the program. It checks the
command-line arguments and calls 'read_file_send_to_fifo' to read data from a
file and send it to a FIFO. The program expects two command-line arguments:
the name of the FIFO file and the name of the file to read data from. If the
user does not provide these arguments, the program displays the correct usage
and exits.

The ERR macro is defined to handle errors. It prints the error message, the
file name, and the line number where the error occurred, then exits the
program.

The 'usage' function is used to display the correct usage of the program if
the user provides incorrect arguments.
*/

#include "client.h"

#include <stdlib.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MSG_SIZE PIPE_BUF - sizeof(pid_t)
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

int read_file_send_to_fifo(char* fifo_name, char* file_name)
{
    // Messages need to be sent with the process pid at front;
    ssize_t count;
    char buffer[PIPE_BUF];
    __pid_t pid = getpid();
    *((pid_t*) buffer) = pid; // Set the pid at the front of the buffer
    size_t offset = sizeof(pid_t);
    char* msg_pointer = buffer + offset; // Set pointer to the end of the pid in buffer

    int fd = open(file_name, O_RDONLY);
    if(fd < 0) ERR("open");

    int fifo = open(fifo_name, O_WRONLY);
    if(fifo < 0) ERR("open");

    while((count = read(fd, msg_pointer, MSG_SIZE)) > 0)
    {
        if(count < MSG_SIZE) memset(msg_pointer + count, 0, MSG_SIZE - count);
        if((write(fifo, buffer, PIPE_BUF)) < 0) ERR("write");
    }

    if(close(fd) < 0) ERR("close");
    if(close(fifo) < 0) ERR("close");
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    if (argc != 3)  usage(argv[0]);
    char* fifo_name = argv[1];
    char* file_name = argv[2];

    read_file_send_to_fifo(fifo_name, file_name);
    return EXIT_SUCCESS;
}