/*
 * This C program simulates a scenario in a kindergarten where children can get sick from a contagious disease, showcasing the use of UNIX signals for inter-process communication and synchronization. It models a simple epidemic spread among children, where one child starts as sick and can potentially infect others based on a probability mechanism.
 *
 * The program is structured to handle multiple children processes, each representing a child in the kindergarten. At the beginning, one child is sick and periodically "coughs" by sending SIGUSR1 signals to the parent process, which can then be forwarded to other children, simulating the spread of the disease. Each child has a probability of getting sick upon being "coughed at", modeled by random number generation.
 *
 * Signal handlers are used to manage the simulation's flow, such as ending the simulation with an alarm signal (SIGALRM), handling cough signals (SIGUSR1), and managing the condition when parents come to pick up a sick child. The use of sigaction for setting signal handlers allows for additional information (like the sender's PID) to be accessed within the signal handling functions.
 *
 * The program utilizes system calls like fork() for creating child processes, kill() for sending signals between processes, and waitpid() for parent processes to wait for child processes to exit, demonstrating process management. It also features custom error handling through the ERR macro and uses dynamic memory allocation for tracking each child's state.
 *
 * In essence, this program is a demonstration of IPC (Inter-Process Communication) using signals in a UNIX environment, simulating a simplistic spread of illness with random elements and process synchronization and communication.
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define UNUSED(x) (void)(x)

volatile sig_atomic_t end_simulation = 0;
volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t cougher_pid = 0;
volatile sig_atomic_t parents_came = 0;

void alarm_handler(int signum)
{
    if (signum == SIGALRM)
    {
        end_simulation = 1;
    }
}

void sigusr1_handler(int signum, siginfo_t *siginfo, void *ucontext)
{
    UNUSED(ucontext);
    if (signum == SIGUSR1)
    {
        last_signal = SIGUSR1;
        cougher_pid = siginfo->si_pid;
    }
}
void child_alarm_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        parents_came = 1;
    }
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}
void setaction(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void usage(const char *pname)
{
    fprintf(stderr, "USAGE: %s t k n p\n", pname);
    fprintf(stderr, "\tt - simulation time in seconds (1-100)\n");
    fprintf(stderr, "\tk - time after ill child is picked up by their parents (1-100)\n");
    fprintf(stderr, "\tn - number of children\n");
    fprintf(stderr, "\tp - likelihood of contracting the disease after contact with the virus\n");
    exit(EXIT_FAILURE);
}
void child_work(int sick, int k, int p)
{
    int n_coughs = 0;
    // Sleep random time
    srand(time(NULL) * getpid());
    int rand_time_ms = 300 + rand() % (1000 - 300 + 1);
    errno = 0;
    for (tt = usleep(rand_time_ms * 1000); tt > 0; tt = usleep(tt))
    {
    }
    int roll_for_sickness;
    for (;;)
    {
        if (sick)
        {
            alarm(k);
            int rand_cough_interval_ms = 50 + rand() % (200 - 50 + 1);
            int tt;
            for (tt = usleep(rand_cough_interval_ms * 1000); tt > 0; tt = usleep(tt))
            {
            }
            n_coughs++;
            printf("Child[%d] is coughing (%d)\n", getpid(), n_coughs);
            kill(0, SIGUSR1);  // Cough
        }
        else
        {
            pause();
        }
        if (last_signal == SIGUSR1)
        {
            printf("%d has coughed at me [%d]\n", cougher_pid, getpid());
            roll_for_sickness = 1 + rand() % (100 - 1 + 1);
            if (roll_for_sickness > p && sick != 1)
            {
                sick = 1;
                printf("Child[%d] got sick!\n", getpid());
            }
            last_signal = 0;
        }
        if (parents_came)
            printf("Parents came for [%d]\n", getpid());
        exit(n_coughs);
    }
}

struct children_stats
{
    int n;
    int pid;
    int n_coughs;
};

void create_children(int n, int k, int p, struct children_stats *result)
{
    pid_t s;
    int i;
    for (i = 0; i < n; i++)
    {
        int sick = 0;
        if (i == 0)
            sick = 1;
        s = fork();
        switch (s)
        {
            case 0:
                // child
                printf("Child[%d] Starts day in the kindergarten, ill: %d\n", getpid(), sick);
                setaction(sigusr1_handler, SIGUSR1);
                sethandler(child_alarm_handler, SIGALRM);
                child_work(sick, k, p);
                exit(EXIT_SUCCESS);
            case -1:
                ERR("Can't make child");
            default:
                // parent
                result[i].n = i;
                result[i].pid = s;
                result[i].n_coughs = 0;
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 5)
        usage(argv[0]);
    int t = atoi(argv[1]);
    if (t < 1 || t > 100)
    {
        usage(argv[0]);
    }
    int k = atoi(argv[2]);
    if (k < 1 || k > 100)
    {
        usage(argv[0]);
    }
    int n = atoi(argv[3]);
    if (n < 1 || n > 30)
    {
        usage(argv[0]);
    }
    int p = atoi(argv[4]);
    if (t < 1 || t > 100)
    {
        usage(argv[0]);
    }
    struct children_stats *result = (struct children_stats *)malloc(sizeof(struct children_stats) * n);
    create_children(n, k, p, result);
    sethandler(alarm_handler, SIGALRM);
    sethandler(SIG_IGN, SIGUSR1);
    printf("Alarm has been set for %d\n", t);
    alarm(t);

    int n_2 = n;

    // Wait children
    while (n > 0)
    {
        pid_t pid;
        int status;
        for (;;)
        {
            if (end_simulation)
            {
                printf("Simulation time is up: kill all children\n");
                while (n_2-- > 0)
                {
                    kill(result[n_2].pid, SIGTERM);
                }
                exit(EXIT_SUCCESS);
            }
            pid = waitpid(0, &status, WNOHANG);
            if (pid > 0)
            {
                // If waited a child;
                n--;
            }
            if (0 == pid)
                break;
            if (0 >= pid)
            {
                if (ECHILD == errno)
                    break;
                ERR("waitpid:");
            }
        }
    }

    exit(EXIT_SUCCESS);
}
