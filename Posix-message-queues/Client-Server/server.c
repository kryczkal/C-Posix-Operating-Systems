//
// Created by wookie on 3/19/24.
//

/*
 * This program consists of two parts: a client and a server, which communicate
 * using POSIX message queues.
 *
 * The server waits for the following data from any queue: the client's PID and
 * two integers. It then calculates the result - the sum for the PID_s queue,
 * the quotient for the PID_d queue, and the remainder of the division for the
 * PID_m queue. The result is written to the client's queue (see the client's
 * description below). Upon receiving the SIGINT signal, the server removes its
 * queues and terminates.
 *
 * The server creates its queues and prints their names. After a second, it
 * removes them and terminates. The client creates its queue, waits for 1
 * second, removes it, and terminates.
 *
 * The server reads the first message from the PID_s queue, then sends a
 * response to the client. At this stage, the program ignores all errors. The
 * client reads 2 numbers from the standard input and sends a message to the
 * server. It waits for a response and prints it.
 *
 * The server handles all queues. It terminates after receiving SIGINT. The
 * client sends its messages until EOF is read or the response waiting time is
 * exceeded.
 *
 * The queues are removed when the programs are closed. Full error handling is
 * implemented.
 */

#include <unistd.h>
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
#define MAYBE_UNUSED(x) (void)(x)
#define MSG_QUEUE_NAME_SIZE 256
#define MSG_SERVER_SIZE sizeof(pid_t) + 2 * sizeof(int64_t)

#define NUM_OPERATIONS 3
#define SUMMATION_OPERATION 0
#define DIVISION_OPERATION 1
#define MODULO_OPERATION 2

void create_message_queues(mqd_t *mq_s, mqd_t *mq_d, mqd_t *mq_m, char *name_mq_s, char *name_mq_d, char *name_mq_m);

void cleanup_message_queues(mqd_t mq_s, mqd_t mq_d, mqd_t mq_m, char *name_mq_s, char *name_mq_d, char *name_mq_m);

void handler_thread(union sigval sv);

volatile sig_atomic_t m_should_work = 1;

typedef struct handler_thread_args_t {
    mqd_t mq;
    int8_t operation;
} handler_thread_args_t;

void set_handler(int sig, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(sig, &sa, NULL) == -1) ERR("sigaction");
}

void sigint_handler(int sig) {
    m_should_work = 0;
}

int main(int argc, char** argv){
    MAYBE_UNUSED(argc);
    MAYBE_UNUSED(argv);

    mqd_t mq_s;
    mqd_t mq_d;
    mqd_t mq_m;
    char name_mq_s[MSG_QUEUE_NAME_SIZE];
    char name_mq_d[MSG_QUEUE_NAME_SIZE];
    char name_mq_m[MSG_QUEUE_NAME_SIZE];

    set_handler(SIGINT, sigint_handler);
    create_message_queues(&mq_s, &mq_d, &mq_m, name_mq_s, name_mq_d, name_mq_m);

    handler_thread_args_t args_s[NUM_OPERATIONS] = {
            {
                    .mq = mq_s,
                    .operation = SUMMATION_OPERATION
            },
            {
                    .mq = mq_d,
                    .operation = DIVISION_OPERATION
            },
            {
                    .mq = mq_m,
                    .operation = MODULO_OPERATION
            }
    };
    struct sigevent sev[NUM_OPERATIONS] = {
            {
                    .sigev_notify = SIGEV_THREAD,
                    .sigev_notify_function = handler_thread,
                    .sigev_notify_attributes = NULL,
                    .sigev_value.sival_ptr = (void*)&args_s[0]
            },
            {
                    .sigev_notify = SIGEV_THREAD,
                    .sigev_notify_function = handler_thread,
                    .sigev_notify_attributes = NULL,
                    .sigev_value.sival_ptr = (void*)&args_s[1]
            },
            {
                    .sigev_notify = SIGEV_THREAD,
                    .sigev_notify_function = handler_thread,
                    .sigev_notify_attributes = NULL,
                    .sigev_value.sival_ptr = (void*)&args_s[2]
            }
    };
    for(int i = 0; i < NUM_OPERATIONS; i++){
        mq_notify(args_s[i].mq, &sev[i]);
    }

    while(m_should_work){
        pause();
    }

    cleanup_message_queues(mq_s, mq_d, mq_m, name_mq_s, name_mq_d, name_mq_m);

    return EXIT_SUCCESS;
}

void create_message_queues(mqd_t *mq_s, mqd_t *mq_d, mqd_t *mq_m, char *name_mq_s, char *name_mq_d, char *name_mq_m) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    // The message size is set to the size of the pid_t type and two int64_t types
// because the message will contain the PID of the client and two integers to operate on
    attr.mq_msgsize = MSG_SERVER_SIZE;

    // Set the message queue name to PID_'char'
    sprintf(name_mq_s, "/%d_s", getpid());
    sprintf(name_mq_d, "/%d_d", getpid());
    sprintf(name_mq_m, "/%d_m", getpid());

    if (((*mq_s) = mq_open(name_mq_s, O_CREAT | O_RDWR, 0666, &attr) ) == (mqd_t) -1) ERR("mq_open");
    if (((*mq_d) = mq_open(name_mq_d, O_CREAT | O_RDWR, 0666, &attr) ) == (mqd_t) -1) ERR("mq_open");
    if (((*mq_m) = mq_open(name_mq_m, O_CREAT | O_RDWR, 0666, &attr) ) == (mqd_t) -1) ERR("mq_open");

    printf("Server PID: %d\n", getpid());
    printf("Server summation message queue name: %s\n", name_mq_s);
    printf("Server division message queue name: %s\n", name_mq_d);
    printf("Server modulo message queue name: %s\n", name_mq_m);
}

void cleanup_message_queues(mqd_t mq_s, mqd_t mq_d, mqd_t mq_m, char *name_mq_s, char *name_mq_d, char *name_mq_m) {
    if(mq_close(mq_s) == -1) ERR("mq_close");
    if(mq_close(mq_d) == -1) ERR("mq_close");
    if(mq_close(mq_m) == -1) ERR("mq_close");
    if(mq_unlink(name_mq_s) == -1) ERR("mq_unlink");
    if(mq_unlink(name_mq_d) == -1) ERR("mq_unlink");
    if(mq_unlink(name_mq_m) == -1) ERR("mq_unlink");
}

void handler_thread(union sigval sv) {
    handler_thread_args_t *args = (handler_thread_args_t*)sv.sival_ptr;
    mqd_t mq = args->mq;
    int8_t operation = args->operation;

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handler_thread;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = sv.sival_ptr;

    mq_notify(mq, &sev);

    // Initialize variables
    char client_mq_name[MSG_QUEUE_NAME_SIZE];
    char buffer[MSG_SERVER_SIZE];
    mqd_t client_mq;

    // Receive the message from the client
    if (mq_receive(mq, buffer, MSG_SERVER_SIZE, NULL) != MSG_SERVER_SIZE) ERR("mq_receive");
    pid_t client_pid = *((pid_t*)buffer);
    int64_t a = *((int64_t*)(buffer + sizeof(pid_t)));
    int64_t b = *((int64_t*)(buffer + sizeof(pid_t) + sizeof(int64_t)));

    // Log the received message
    printf("Server received request from client with PID: %d\n", client_pid);
    printf("Server received request with arguments: %ld, %ld\n", a, b);
    printf("Server received request with operation: %d\n", operation);

    // Perform the operation
    int64_t result;
    if(operation == SUMMATION_OPERATION) {
        result = a + b;
    } else if(operation == DIVISION_OPERATION) {
        if(b == 0) {
            result = 0;
        } else {
            result = a / b;
        }
    } else if(operation == MODULO_OPERATION) {
        if(b == 0) {
            result = 0;
        } else {
            result = a % b;
        }
    } else {
        ERR("Invalid operation");
    }

    // Set the message queue name of the client to its PID
    sprintf(client_mq_name, "/%d", client_pid);

    // Open the client_mq message queue and send the result then close the message queue
    if((client_mq = mq_open(client_mq_name, O_RDWR)) == (mqd_t) -1) ERR("mq_open");
    if (mq_send(client_mq, (char*)&result, sizeof(int64_t), 0) == -1) ERR("mq_send");
    if (mq_close(client_mq) == -1) ERR("mq_close");
}
