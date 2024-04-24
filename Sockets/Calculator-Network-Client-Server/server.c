//
// Created by wookie on 4/19/24.
//
#include "socklib.h"
#include "macros.h"

#define MESSAGE_SIZE 5
#define MAX_EVENTS 100

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    do_work = 0;
}

void usage(char *name)
{
    fprintf(stderr, "Usage: %s <local socket name> <port number>\n", name);
    fprintf(stderr, "  port number: 1-65535\n");
    exit(EXIT_FAILURE);
}

/// @brief Perform a calculation based on the data received from the client and store it in the data array,
/// converting the data to host byte order and back to network byte order
/// @param data Data received from the client
void perform_calculation(int32_t* data)
{
    // Convert the data to host byte order
    for(int i = 0; i < 5; i++) data[i] = ntohl(data[i]);

    // Perform the calculation
    switch((char)data[OPERATION_INDEX]){
        case '+':
            data[RESULT_INDEX] = data[OPERAND1_INDEX] + data[OPERAND2_INDEX];
            break;
        case '-':
            data[RESULT_INDEX] = data[OPERAND1_INDEX] - data[OPERAND2_INDEX];
            break;
        case '*':
            data[RESULT_INDEX] = data[OPERAND1_INDEX] * data[OPERAND2_INDEX];
            break;
        case '/':
            if(data[OPERAND2_INDEX] == 0){
                data[STATUS_INDEX] = -1;
            } else {
                data[RESULT_INDEX] = data[OPERAND1_INDEX] / data[OPERAND2_INDEX];
            }
            break;
        default:
            data[STATUS_INDEX] = -1;
    }

    // Convert the data back to network byte order
    for(int i = 0; i < 5; i++) data[i] = htonl(data[i]);
}

int server_work(int tcp_socket_fd, int local_socket_fd)
{
    /*
     * Create an epoll instance and add the TCP and local sockets to it.
     */

    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        ERR("epoll_create");

    // Add the TCP and local sockets to the epoll instance
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = tcp_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket_fd, &ev) < 0)
        ERR("epoll_ctl");
    ev.data.fd = local_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, local_socket_fd, &ev) < 0)
        ERR("epoll_ctl");

    // Set up the maximum number of events and live connections
    int N_LIVE_CONNECTIONS = 2;

    // Create an array of epoll events
    struct epoll_event events[MAX_EVENTS];

    // Create an array of data to be sent and received to/from the client
    int32_t data[MESSAGE_SIZE];
    ssize_t received_message_size;

    // Create a sigmask for SIGINT
    sigset_t sigmask, old_mask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGINT);
    sigprocmask(SIG_BLOCK, &sigmask, &old_mask);

    // Main server loop
    while(do_work){
        // Wait for events
#ifdef DEBUG
        fprintf(stderr, "Waiting for events\n");
#endif
        int nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, &old_mask);
        if (nfds < 0 && errno != EINTR)
            ERR("epoll_wait");

#ifdef DEBUG
        fprintf(stderr, "Received %d events\n", nfds);
#endif

        for (int i = 0; i < nfds; i++)
        {
            // Accept a new connection
            int client_fd = add_new_client(events[i].data.fd);
            if (client_fd < 0)
                continue;

            // Receive data from the client
            if ( (received_message_size = bulk_read(client_fd, (char *) data, sizeof(int32_t[5])) ) < 0)
            {
                ERR("read");
            }
#ifdef DEBUG
            fprintf(stderr, "Received message of size %ld\n", received_message_size);
#endif

            if (received_message_size == sizeof(data)){
                // Perform the calculation and send the result back to the client
                perform_calculation(data);
#ifdef DEBUG
                fprintf(stderr, "Operand1: %d, Operand2: %d, Result: %d, Operation: %c, Status: %d\n",
                        ntohl(data[OPERAND1_INDEX]), ntohl(data[OPERAND2_INDEX]), ntohl(data[RESULT_INDEX]), (char)ntohl(data[OPERATION_INDEX]), ntohl(data[STATUS_INDEX]));
#endif
                if (TEMP_FAILURE_RETRY(write(client_fd, data, sizeof(int32_t[5]))) < 0)
                    ERR("write");
            } else {
                fprintf(stderr, "Received partial message\n");
            }

            // Close the client socket
            if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
                ERR("close");
        }
    }

    // Close the epoll instance
    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    // Parse command line arguments (local socket name and port number)
    if (argc != 3)
        usage(argv[0]);
    char *name = argv[1];
    uint16_t port = atoi(argv[2]);
    if (port < 1 || port > 65535)
        usage(argv[0]);

    sethandler(sigint_handler, SIGINT); // Handle SIGINT
    sethandler(SIG_IGN, SIGPIPE); // Ignore SIGPIPE

    // Create a TCP socket, bind it to a port and start listening, set it to non-blocking mode
    int tcp_socket_fd = bind_tcp_socket(port, SOMAXCONN);
    int tcp_socket_flags = fcntl(tcp_socket_fd, F_GETFL) | O_NONBLOCK;
    if (tcp_socket_flags < 0)
        ERR("fcntl");
    if (fcntl(tcp_socket_fd, F_SETFL, tcp_socket_flags) < 0)
        ERR("fcntl");

    fprintf(stderr, "Listening on port %d\n", port);

    // Create a local socket, bind it to a name and start listening, set it to non-blocking mode
    int local_socket_fd = bind_local_socket(name, SOMAXCONN);
    int local_socket_flags = fcntl(local_socket_fd, F_GETFL) | O_NONBLOCK;
    if (local_socket_flags < 0)
        ERR("fcntl");
    if (fcntl(local_socket_fd, F_SETFL, local_socket_flags) < 0)
        ERR("fcntl");

    fprintf(stderr, "Listening on local socket %s\n", name);

    // Start the server work
    server_work(tcp_socket_fd, local_socket_fd);

    // Close the TCP and local sockets
    if (TEMP_FAILURE_RETRY(close(tcp_socket_fd)) < 0)
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(local_socket_fd)) < 0)
        ERR("close");

    // Unlink the local socket
    unlink_local_socket(name);

    fprintf(stderr, "Server finished\n");
    return EXIT_SUCCESS;
}