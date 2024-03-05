//
// Created by wookie on 3/5/24.
//

/*
This program is a multi-process application written in C that demonstrates
inter-process communication using pipes. It creates a specified number of
child processes and establishes a communication channel with each of them
using pipes.

The parent process waits for a SIGINT signal (usually triggered by Ctrl+C).
Upon receiving the signal, it selects a random child process and sends a
random character to it. The child process, upon receiving the character,
decides whether to terminate itself (with a 20% chance) or to send a message
back to the parent. The message consists of the received character repeated a
random number of times.

The child processes also wait for a SIGINT signal. If they receive it, they
decide whether to terminate themselves or to continue running. If a child
process decides to terminate, it closes its end of the pipe, which allows the
parent process to detect the termination.

The parent process reads the messages from the children from a single pipe. It
continues to do so until it has no more children (all of them decided to
terminate).

The program uses signal handlers to react to SIGINT and SIGCHLD signals. The
SIGCHLD signal is sent to a parent process when one of its child processes
terminates. The program uses a handler for this signal to reap the terminated
child processes and prevent them from becoming zombies.

The program also handles SIGPIPE signal, which is sent to a process that
attempts to write to a pipe with no readers. This can happen if a child
process terminates while the parent is still trying to write to it. The
program ignores this signal to prevent it from terminating the parent process.

The program takes one command-line argument, which is the number of child
processes to create. It checks the validity of this argument and displays a
usage message if it's not valid.

The program uses dynamic memory allocation to store the data about the child
processes. It allocates an array of structures, each containing the process ID
and the file descriptor of the write end of the pipe to a child process. The
program frees this memory before it terminates.
*/

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/wait.h>
#include <errno.h>
#include <linux/limits.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define CHILD_MAX_MSG_SIZE 100

volatile sig_atomic_t parent_received_signal = 0;
volatile sig_atomic_t child_received_signal = 0;

typedef struct child_data_t {
    int fd;
    int pid;
}child_data_t;

void usage(char* name){
    fprintf(stderr, "USAGE: %s n\n", name);
    exit(EXIT_FAILURE);
}

int setHandler(void (*f)(int), int sigNo) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = f;
    if (sigaction(sigNo, &action, NULL) == -1) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

void sig_int_parent_handler(int sig){
    parent_received_signal = 1;
}

void sig_int_child_handler(int sig){
    child_received_signal = 1;
}

void sigchld_handler(int sig){
    pid_t pid;
    for(;;) {
        pid = waitpid(0, NULL, WNOHANG);
        if(pid < 0) {
            if(errno == ECHILD) break;
            ERR("waitpid");
        }
        if(pid == 0) return;
    }
}

void parent_work(int read_pipe, child_data_t **child_data, int n_children){
    char buffer[PIPE_BUF];
    for(;;){
        if(parent_received_signal){
            fprintf(stderr, "Parent [%d]: received signal\n", getpid());
            fprintf(stderr, "Parent [%d]: sending random char to a random child\n", getpid());
            // Find a random child to send to
            int random_child_id = rand() % n_children;
            while((*child_data)[random_child_id % n_children].fd == -1 && random_child_id < 2 * n_children) random_child_id++;
            // If there are no children to send to, exit
            if((*child_data)[random_child_id].fd == 0) {
                fprintf(stderr, "Parent [%d]: no children to send to, exiting\n", getpid());
                break;
            }

            // Send a random char to a random child
            fprintf(stderr, "Parent [%d]: sending random char to child [%d]\n", getpid(), (*child_data)[random_child_id].pid);
            char random_char = (char) ('a' + rand() % ('z' - 'a'));
            if(TEMP_FAILURE_RETRY(
                    write( (*child_data)[random_child_id].fd,
                          &random_char,
                          sizeof(char)) ) != sizeof(char) ) {
                if(errno == EPIPE){
                    fprintf(stderr, "Parent [%d]: child [%d] is dead, removing it\n", getpid(), (*child_data)[random_child_id].pid);
                    if(close((*child_data)[random_child_id].fd)) ERR("close");
                    (*child_data)[random_child_id].fd = 0;
                } else {
                    ERR("write");
                }
            }

            fprintf(stderr, "Parent [%d]: waiting for message from children\n", getpid());
            int status = read(read_pipe, buffer, PIPE_BUF);
            if(status < 0 && errno == EINTR) {
                fprintf(stderr, "Parent [%d]: read interrupted, continuing\n", getpid());
                continue;
            }
            if(status < 0) ERR("read");
            if(status == 0) {
                fprintf(stderr, "Parent [%d]: no children left, exiting\n", getpid());
                break;
            }

            pid_t sender_pid = *((pid_t*) buffer);
            fprintf(stderr, "Parent [%d]: received message from child [%d]: %s\n", getpid(), sender_pid, buffer + sizeof(pid_t));
            parent_received_signal = 0;
        }
    }
    close(read_pipe); // Close the read end of the pipe to the parent
}

void child_work(int fd[2]) {
    srand(getpid());
    char c;
    char buffer[PIPE_BUF];
    // PIPE_BUF depends on the system, so we need to check it
    size_t max_msg_size = PIPE_BUF < CHILD_MAX_MSG_SIZE ? PIPE_BUF - sizeof(pid_t) : 1200 - sizeof(pid_t);
    size_t rolled_msg_size;
    for(;;) {
        if(child_received_signal){
            int roll_for_death = rand() % 100;
            if(roll_for_death <= 20){
                fprintf(stderr, "Child [%d]: received signal, exiting\n", getpid());
                break;
            }


            if(TEMP_FAILURE_RETRY(
                    read(fd[0], &c, sizeof(char) ) ) != sizeof(char) ) {
                if(close(fd[0])) ERR("close");
                break;
            }
            fprintf(stderr, "Child [%d]: received char: %c\n", getpid(), c);
            fprintf(stderr, "Child [%d]: rolling for message size\n", getpid());
            rolled_msg_size = rand() % max_msg_size;
            // Set the pid at the front of the buffer
            *((pid_t*) buffer) = getpid();

            // Fill the buffer with the random char
            memset(buffer + sizeof(pid_t), c, rolled_msg_size);
            // Fill the rest of the buffer with 0s
            memset(buffer + sizeof(pid_t) + rolled_msg_size, 0, max_msg_size - rolled_msg_size);
            fprintf(stderr, "Child [%d]: sending message of size %ld\n", getpid(), max_msg_size);
            if(write(fd[1], buffer, max_msg_size) == -1) ERR("write");

            child_received_signal = 0;
        }
    }
    close(fd[0]); // Close the read end of the pipe
    close(fd[1]); // Close the write end of the pipe
}

void create_children(int n_children, child_data_t** child_data_array, int pipe_to_parent[2])
{
    for(int i = 0; i < n_children; i++){
        pid_t pid;
        int pipe_to_child[2]; // this is the pipe that the parent will use to write to the children

        if(pipe(pipe_to_child) == -1){
            ERR("pipe");
        }
        switch(pid = fork()){
            case -1:
                ERR("fork");
            case 0:
                // Child
                if(close(pipe_to_child[1])) ERR("close"); // Close the write end of the pipe_to_child

                if(setHandler(sig_int_child_handler, SIGINT)) ERR("setHandler");
                int fd[2] = {pipe_to_child[0], pipe_to_parent[1]};
                child_work(fd); // the child will read from pipe_to_child[0] and write to pipe_to_parent[1]
                exit(EXIT_SUCCESS);
            default:
                // Parent
                if(close(pipe_to_child[0])) ERR("close"); // Close the read end of the pipe
                (*child_data_array)[i].pid = pid; // Save the pid of the ith child
                (*child_data_array)[i].fd = pipe_to_child[1]; // Save the write end of the pipe to the ith child
                break;
        }
    }
    if(close(pipe_to_parent[1])) ERR("close"); // Close the write end of the pipe_to_parent
}

void free_child_data(child_data_t* child_data, int n_children){
    free(child_data);
}

int main(int argc, char** argv) {
    if (argc != 2) usage(argv[0]);
    int n_children = atoi(argv[1]);
    // Create a list of file descriptors to a pipe that will be used to communicate with the children
    child_data_t *child_data = malloc(n_children * sizeof(child_data_t));

    int pipe_to_parent[2]; // this is the pipe that the children will use to write to the parent
    if (pipe(pipe_to_parent) == -1) ERR("pipe");

    if (setHandler(sig_int_parent_handler, SIGINT)) ERR("setHandler");
    if (setHandler(sigchld_handler, SIGCHLD)) ERR("setHandler");
    if(setHandler(SIG_IGN, SIGPIPE)) ERR("setHandler");
    create_children(n_children, &child_data, pipe_to_parent);
    parent_work(pipe_to_parent[0], &child_data, n_children);

    free_child_data(child_data, n_children);
    return EXIT_SUCCESS;
}