/*
 * This program requires two command-line arguments: M (milliseconds) and N (number of cycles before sending SIGUSR2), to control the child process's behavior.
 *
 * The main process sets up signal handlers for SIGUSR1, SIGUSR2, and SIGCHLD using the setHandler function.
 * It then creates a child process using fork.
 * The child process, upon creation, enters a loop where it sends SIGUSR1 signals to the parent process at intervals
 * specified by the M argument. After N cycles, it sends a SIGUSR2 signal.
 * The parent process waits for signals, specifically SIGUSR2, using sigsuspend, which pauses the process until a signal
 * is received. Upon receiving SIGUSR2, it prints a message including the total count of SIGUSR2 signals received.
 *
 * Signal handling setup involves blocking SIGUSR1 and SIGUSR2 signals during initialization to prevent handling before
 * the program is ready. The original signal mask is restored before entering the main loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <errno.h>

#define ERR(str) (fprintf(stderr, "Error occurred on line: %d in file: %s\n", __LINE__, __FILE__), perror(str) \
        ,kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t u1Sig = 0;
volatile sig_atomic_t u2Sig = 0;

 void setHandlerPar(int sigNum, void (*f)(int), int Flags, sigset_t mask)
 {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));

    act.sa_handler = f;
    if (sigaction(sigNum, &act, NULL))
        ERR("Sigaction");
 }

 void defaultHandlerU1(int sigNum){
    u1Sig = 1;
}

void defaultHandlerU2(int sigNum){
     u2Sig =1;
 }

void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}

 void setHandler(int sigNum, void (*f)(int)){
    sigset_t mask;
    sigemptyset(&mask);

    setHandlerPar(sigNum, f, 0, mask);
}

void parentJob(sigset_t mask){
    int u2Total = 0;
     while(1){
        u2Sig = 0;
        while(u2Sig != 1)
            sigsuspend(&mask);

        printf("[ PARENT: %d ] Received SIGUSR2 signal...\nTotal count: %d\n", getpid(), ++u2Total);
     }
}

void childJob(int m, int n)
{
     static const int interSleep = 100000;
    static const int SEC_TO_MSEC = 1000;
    static const int MSEC_TO_NSEC = 1000000;
    int seconds = m / SEC_TO_MSEC;
    int nSeconds = (m % SEC_TO_MSEC) * MSEC_TO_NSEC;
    pid_t pPid = getppid();
    int u2Send = 0;

    int cycles = 0;
    while(1)
    {
        struct timespec tm = { .tv_nsec = nSeconds, .tv_sec = seconds };
        while(nanosleep(&tm, &tm));

        if (kill(getppid(), SIGUSR1))
            ERR("kill");

        if (++cycles == n){
            struct timespec t = {.tv_sec = 0, .tv_nsec = interSleep};
//            nanosleep(&t, NULL);

            if (kill(getppid(), SIGUSR2))
                ERR("kill");

            printf("[ CHILD: %d ] Send SIGUSR2 to parent %d...\nTotal amount send: %d\n", getpid(), getppid(), ++u2Send);
            cycles = 0;
        }
    }
}

void spawnChild(int m, int n)
{
    int status = fork();

    if (status < 0){
        ERR("fork");
    }
    if (status == 0){
        childJob(m, n);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char** argv)
{
    int m, n;
    if (argc != 3){
        fprintf(stderr , "[ ERROR ] Expected 2 arguments received less or more\n");
        exit(EXIT_FAILURE);
    }

    m = atoi(argv[1]); n = atoi(argv[2]);

    if (n <= 0 || m <= 0){
        fprintf(stderr, "[ ERROR ] Passed arguments are not valid\n");
        exit(EXIT_FAILURE);
    }
    setHandler(SIGUSR1, defaultHandlerU1);
    setHandler(SIGUSR2, defaultHandlerU2);
    setHandler(SIGCHLD, sigchld_handler);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    spawnChild(m, n);
    parentJob(oldmask);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    while(wait(NULL)>0);
    return EXIT_SUCCESS;
}