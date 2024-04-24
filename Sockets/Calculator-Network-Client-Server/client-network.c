//
// Created by wookie on 4/19/24.
//

#include "socklib.h"
#include "macros.h"

int main(int argc, char **argv)
{
    int32_t data[5];
    /*
     * Parse arguments (server address, port number, operand 1, operand 2, operation)
     */

    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <server_address> <port> <operand1> <operand2> <operation>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    data[OPERAND1_INDEX] = atoi(argv[3]);
    if (data[OPERAND1_INDEX] < 0 || data[OPERAND1_INDEX] > INT32_MAX)
    {
        fprintf(stderr, "Operand 1 must be a positive integer\n");
        exit(EXIT_FAILURE);
    }
    data[OPERAND2_INDEX] = atoi(argv[4]);
    if (data[OPERAND2_INDEX] < 0 || data[OPERAND2_INDEX] > INT32_MAX)
    {
        fprintf(stderr, "Operand 2 must be a positive integer\n");
        exit(EXIT_FAILURE);
    }
    data[OPERATION_INDEX] = argv[5][0];
    if (data[OPERATION_INDEX] != '+' && data[OPERATION_INDEX] != '-' && data[OPERATION_INDEX] != '*' && data[OPERATION_INDEX] != '/')
    {
        fprintf(stderr, "Operation must be one of the following: +, -, *, /\n");
        exit(EXIT_FAILURE);
    }
    data[STATUS_INDEX] = 0;
    data[RESULT_INDEX] = 0;


    /*
     * Connect to the server
     */

    int tcp_socket_fd = connect_tcp_socket(argv[1], argv[2]);

    /*
     * Send the data to the server
     */

    // Prepare the data to be sent
    for(int i = 0; i < 5; i++) data[i] = htonl(data[i]);

    // Ignore SIGINT for the time of sending and receiving the data
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    if (TEMP_FAILURE_RETRY(bulk_write(tcp_socket_fd, (char *)data, sizeof(data))) < 0)
    {
        ERR("write");
    }


    /*
     * Receive the data from the server
     */

    if (TEMP_FAILURE_RETRY(bulk_read(tcp_socket_fd, (char *)data, sizeof(data))) < 0) {
        ERR("read");
    }
    // Restore the old mask
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    // Convert the data to host byte order
    for (int i = 0; i < 5; i++) data[i] = ntohl(data[i]);

    // Print the result
    if (data[STATUS_INDEX] == 0)
    {
        printf("Result: %d\n", data[RESULT_INDEX]);
    }
    else
    {
        fprintf(stderr, "Error: %d\n", data[RESULT_INDEX]);
    }

    return EXIT_SUCCESS;
}