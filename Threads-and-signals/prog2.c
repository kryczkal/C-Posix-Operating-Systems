/*
 * This progrma creates multiple child processes, each performing a specific job based on signal reception, and a
 * parent process that coordinates and controls these child processes through signals.
 * 
 * The main process takes four command-line arguments:
 * n (number of child processes),
 * k (seconds before sending SIGUSR1),
 * p (seconds before sending SIGUSR2),
 * r (number of repetitions for child processes).
 * 
 * Each child process:
 * - Is configured to handle SIGUSR1 and SIGUSR2 signals.
 * - Alternates between sleeping for a random period and checking the last received signal to print success or failure.
 * - Terminates after completing its designated number of repetitions.
 * 
 * The parent process:
 * - Sets up a handler for SIGCHLD to clean up child processes as they terminate.
 * - Uses alarms (SIGALRM) to impose a timeout on the entire operation.
 * - Periodically sends SIGUSR1 and SIGUSR2 to all child processes, alternating between the two signals based on the specified intervals (k and p).
 * - Waits for all child processes to terminate before exiting.
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define ERR(str) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(str), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void procSig(int signo)
{
    printf("[ %d ] Received signal %d\n", getpid(), signo);
    last_signal = signo;
}

void setHandler(void (*f)(int), int sig)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler  = f;
    if (sigaction(sig, &act, NULL) == -1)
        ERR("sigaction");
}

#define SEC_TO_NSEC  (long)1000000000

void processJob(int r){
    srand(getpid());

    while(r --> 0){
        long secondsToSleep = 5 + rand() % 6;
        while((secondsToSleep = sleep(secondsToSleep)));

        printf("[ %d ] %s\n", getpid(), last_signal == SIGUSR1 ? "SUKCES" : "FAILURE");
    }

    printf("[ %d ] Terminates...\n", getpid());
}

void childHandler(int signo)
{
    (void)signo;
    pid_t pid;

    while(1)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0) return;

        if (pid < 0){
            if (errno == ECHILD) return;
            ERR("waitpid");
        }
    }
}

void spawnProcs(int n, int r){
    for (int i = 0; i < n; ++i)
    {
        int forkRes = fork();

        if (forkRes < 0) ERR("Not able to spawn all childs\n");
        else if (forkRes == 0)
        {

            setHandler(procSig, SIGUSR1);
            setHandler(procSig, SIGUSR2);

            processJob(r);
            exit(EXIT_SUCCESS);
        }
    }
}

void parentJob(int n, int k, int p){
    int status;
    setHandler(procSig, SIGALRM);
    alarm(n * 10);

   while(n){
       struct timespec tk = { .tv_sec = k, .tv_nsec = 0 };
       struct timespec tp = { .tv_sec = p, .tv_nsec = 0 };

       while(nanosleep(&tk, &tk));
       if (kill(0, SIGUSR1) < 0){
            ERR("kill");
       }

       while(nanosleep(&tp, &tp));
       if (kill(0, SIGUSR2))
       {
           ERR("Kill");
       }

       while(n && (status = waitpid(-1, NULL, WNOHANG))){
           if (status == -1) ERR("Error occurred during waiting for childs, probably one died earlier");
           --n;
       }
   }
}

int main(int argc, const char** argv)
{
    int n, k, p, r;

    if (argc != 5){
        fprintf(stderr, "Not enough input arguments\n");
        exit(EXIT_FAILURE);
    }

    n = atoi(argv[1]); k = atoi(argv[2]); p = atoi(argv[3]); r = atoi(argv[4]);
    if (n <= 0 || k <= 0 || p <= 0 || r <= 0){
        fprintf(stderr, "Every argument must be a positive integer\n");
        exit(EXIT_FAILURE);
    }

    setHandler(childHandler, SIGCHLD);
    setHandler(SIG_IGN, SIGUSR1);
    setHandler(SIG_IGN, SIGUSR2);

    spawnProcs(n, r);
    parentJob(n, k, p);

    while(wait(NULL) > 0);

    return EXIT_SUCCESS;
}