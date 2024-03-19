//
// Created by wookie on 3/17/24.
//

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#include <sys/wait.h>

#define MAYBE_UNUSED(x) (void)(x)
#define BINGO_MAX_LIVES 3
#define BINGO_MAX_NUMBER 100

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t children_left;

void usage(char* exec_name) {
    fprintf(stderr, "Usage: %s <number of players>\n", exec_name);
    fprintf(stderr, "0 < n < 100\n");
    exit(EXIT_FAILURE);
}

// #TODO
void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = f;
    sa.sa_flags = SA_SIGINFO;
    if(-1 == sigaction(sigNo, &sa, NULL)) ERR("sigaction");
}

void sigchld_handler(int sig, siginfo_t *info, void *ucontext) {
    MAYBE_UNUSED(info);
    MAYBE_UNUSED(sig);
    MAYBE_UNUSED(ucontext);
    pid_t pid;
    for(;;) {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0) break;
        if (pid < 0) {
            if (errno == ECHILD) break;
            ERR("waitpid");
        }
        children_left--;
    }
}

void mq_handler(int sig, siginfo_t *info, void *ucontext) {
    MAYBE_UNUSED(sig);
    MAYBE_UNUSED(ucontext);
    mqd_t *pin;
    u_int8_t actual_number;
    u_int32_t msg_prio;
    pin = (mqd_t*)info->si_value.sival_ptr;

    // set notification for the same process again (it's one-time only)
    static struct sigevent not;
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo = SIGRTMIN;
    not.sigev_value.sival_ptr = pin;
    if(mq_notify(*pin, &not ) < 0) ERR("mq_notify");

    for(;;) {
        if(TEMP_FAILURE_RETRY(mq_receive(*pin, (char*)&actual_number, 1, &msg_prio)) < 1) {
            if(errno == EAGAIN) break;
            ERR("mq_receive");
        }
        if(msg_prio == 0)
            printf("MQ: Got timeout from %d\n", actual_number);
        else
            printf("MQ: %d is a bingo number\n", actual_number);
    }
}

void child_work(int id, mqd_t pin, mqd_t pout) {
    srand(getpid());
    uint lives = rand() % BINGO_MAX_LIVES + 1;
    u_int8_t my_guess = rand() % BINGO_MAX_NUMBER + 1;
    u_int8_t actual_number;
    while(lives-- > 0) {
        // receiving less than 1 byte is an error in this case
        if(TEMP_FAILURE_RETRY(mq_receive(pout, (char*)&actual_number, 1, NULL)) < 1) ERR("mq_receive");
        printf("Child id: %d, pid:[%d] received number: %d\n", id, getpid(), actual_number);
        if(actual_number == my_guess){
            if(TEMP_FAILURE_RETRY(mq_send(pin, (const char*)&my_guess, 1, 1))) ERR("mq_send");
            printf("Child id: %d, pid:[%d] won\n", id, getpid());
            return;
        }
    }
    if (TEMP_FAILURE_RETRY(mq_send(pin, (const char*)& id, 1, 0))) ERR("mq_send");
}

void create_children(int n, mqd_t pin, mqd_t pout) {
    for(int i = 0; i < n; i++) {
        pid_t pid;
        if((pid = fork()) < 0) ERR("fork");
        if(pid == 0) {
            // child
            child_work(i, pin, pout);
            exit(EXIT_SUCCESS);
        }
    }
}

void parent_work(mqd_t pout) {
    srand(getpid());
    u_int8_t actual_answer;
    while(children_left) {
        actual_answer = rand() % BINGO_MAX_NUMBER + 1;
        if(TEMP_FAILURE_RETRY(mq_send(pout, (const char*)& actual_answer, 1, 0))) ERR("mq_send");
        // guarantee that the parent will sleep at least 1 second
        for(int i = sleep(1); i > 0; i = sleep(i));
    }
    printf("[PARENT] terminating\n");
}

int main(int argc, char** argv) {
    MAYBE_UNUSED(argc);
    MAYBE_UNUSED(argv);

    // parse arguments
    if(argc != 2) usage(argv[0]);
    int n = atoi(argv[1]);
    if(n < 1 || n > 100) usage(argv[0]);
    children_left = n;

    // create message queues
    mqd_t pin, pout;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;
    if((pin = TEMP_FAILURE_RETRY(mq_open("/bingo_in",
                                         O_RDWR | O_NONBLOCK | O_CREAT,
                                         0600, &attr))) == (mqd_t)-1) ERR("mq_open");
    if((pout = TEMP_FAILURE_RETRY(mq_open("/bingo_out",
                                          O_RDWR | O_CREAT,
                                          0600, &attr))) == (mqd_t)-1) ERR("mq_open");

    // set handlers
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(mq_handler, SIGRTMIN);

    // create children
    create_children(n, pin, pout);

    // set notification
    static struct sigevent not;
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo = SIGRTMIN;
    not.sigev_value.sival_ptr = &pin;
    if(mq_notify(pin, &not ) < 0) ERR("mq_notify");

    // parent work
    parent_work(pout);

    // cleanup
    if(TEMP_FAILURE_RETRY(mq_close(pin))) ERR("mq_close");
    if(TEMP_FAILURE_RETRY(mq_close(pout))) ERR("mq_close");
    if(TEMP_FAILURE_RETRY(mq_unlink("/bingo_in"))) ERR("mq_unlink");
    if(TEMP_FAILURE_RETRY(mq_unlink("/bingo_out"))) ERR("mq_unlink");

    return EXIT_SUCCESS;
}
