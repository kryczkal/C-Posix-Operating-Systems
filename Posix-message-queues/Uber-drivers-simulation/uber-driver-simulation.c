//
// Created by wookie on 3/20/24.
//

/*
 * This program simulates a car transportation system using POSIX message queues.
 * It takes two arguments: N, which is the number of drivers participating in the
 * simulation, and T, which is the duration of the simulation in seconds.
 *
 * The main process creates N driver processes and waits for them to finish. The
 * drivers randomly generate and print their starting position within the square
 * [-1000, 1000]^2 and then terminate. All position vectors are assumed to have
 * integer coordinates.
 *
 * The main thread creates a queue for new tasks named "uber tasks", and the
 * workers wait for a new task by reading from this queue. When they receive a
 * task (four integer numbers from the range [-1000, 1000]), they change their
 * position to the end of the route, print the task on standard output, sleep for
 * 'd' milliseconds (where 'd' is the distance from the current position to the
 * start of the route plus the length of the route in city metric), and wait for
 * the next task. The main process creates a new task every 500 - 2000
 * milliseconds. If there are already 10 tasks in the queue, the task is rejected
 * and an appropriate message is printed on stderr.
 *
 * After completing a ride, the drivers send information about the completed task
 * to the main process using individual queues named "uber results [PID]". The
 * drivers send the length of the completed ride in city metric through this
 * queue. The main process receives the results and prints them on standard
 * output: "The driver [PID] drove a distance of [distance]".
 *
 * The main process, after creating the children and queues, sets an alarm for T
 * seconds. Upon receiving SIGALRM, it stops sending new tasks, sends a
 * high-priority message to the children, and closes the queue. The drivers
 * detect that the main process has closed the queue by reading a high-priority
 * message and then terminate their work. All resources are properly released,
 * and the queues are closed and removed.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define MAYBE_UNUSED(x) (void)(x)

#define UBER_TASK_MESSAGE_SIZE sizeof(uber_task_t)
#define ENDING_MESSAGE_PRIORITY 10

volatile sig_atomic_t should_run = 1;
sig_atomic_t children_left = 0;

typedef struct uber_task_t {
    int32_t x_start;
    int32_t y_start;
    int32_t x_end;
    int32_t y_end;
} uber_task_t;

typedef struct uber_task_done_message_t {
    int32_t driven_distance;
} uber_task_done_message_t;

typedef struct uber_driver_t {
    pid_t pid;
    int active;
} uber_driver_t;

void set_handler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void alarm_handler(int sig)
{
    MAYBE_UNUSED(sig);
    should_run = 0;
}

void sigchld_handler(int sig)
{
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &mask, &old_mask))
        ERR("sigprocmask");

    MAYBE_UNUSED(sig);
    for(;;) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid < 0) {
            if (errno == ECHILD)
                break;
            ERR("waitpid");
        }
        if (pid == 0)
            break;
        children_left--;
    }

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL))
        ERR("sigprocmask");
}

void usage(const char* name)
{
    fprintf(stderr, "USAGE: %s N T\n", name);
    fprintf(stderr, "N: 1 <= N - number of drivers\n");
    fprintf(stderr, "T: 5 <= T - simulation duration\n");
    exit(EXIT_FAILURE);
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    //https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        ERR("msleep");
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res == -1 && errno == EINTR);
    if(res == -1 && errno != EINTR)
        ERR("nanosleep");

    return res;
}

void children_work(void)
{
    // Set initial coordinates of the driver
    srand(getpid());
    int x, y;
    x = rand() % 2000 - 1000;
    y = rand() % 2000 - 1000;

    printf("Driver %d begins job at: x = %d, y = %d\n", getpid(), x, y);

    // Open the uber_tasks_mq message queue
    mqd_t uber_tasks_mq;
    if ((uber_tasks_mq = mq_open("/uber_tasks", O_RDONLY)) == (mqd_t) -1)
        ERR("mq_open");

    // Open the message queue for the driver
    char uber_results_mq_name[20];
    sprintf(uber_results_mq_name, "/uber_results_%d", getpid());
    mqd_t uber_results_mq;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(uber_task_done_message_t);
    if ((uber_results_mq = mq_open(uber_results_mq_name, O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr)) == (mqd_t) -1)
        ERR("mq_open");


    // Driver loop
    for(;;) {
        // Receive a task
        uber_task_t ride;
        unsigned msg_prio;
        if (mq_receive(uber_tasks_mq, (char*) &ride, UBER_TASK_MESSAGE_SIZE, &msg_prio) == -1)
            ERR("mq_receive");

        if (msg_prio == ENDING_MESSAGE_PRIORITY) {
            printf("Driver %d received an ending message\n", getpid());
            break;
        }

        printf("Driver %d received a task: from: (%d, %d), to :(%d, %d)\n", getpid(),
               ride.x_start, ride.y_start, ride.x_end, ride.y_end);
        // Drive the passenger to the destination
        int driven_distance = 0;
        driven_distance += abs(ride.x_start - x) + abs(ride.y_start - y);
        driven_distance += abs(ride.x_end - ride.x_start) + abs(ride.y_end - ride.y_start);

        // Simulate the ride
        msleep(driven_distance);

        // Update the driver's position
        x = ride.x_end;
        y = ride.y_end;

        // Send the result
        uber_task_done_message_t result;
        result.driven_distance = driven_distance;
        if (mq_send(uber_results_mq, (char*) &result, sizeof(uber_task_done_message_t), 0) == -1)
            ERR("mq_send");
    }

    // Cleanup
    if (mq_close(uber_results_mq) == -1)
        ERR("mq_close");
    if (mq_unlink(uber_results_mq_name) == -1)
        ERR("mq_unlink");

    exit(EXIT_SUCCESS);
}

void create_children(int N, uber_driver_t* uber_drivers)
{
    for (int i = 0; i < N; i++)
    {
        pid_t pid;
        if ( (pid = fork()) < 0)
            ERR("fork");
        if (pid == 0)
        {
            // child
            children_work();
            exit(EXIT_SUCCESS);
        }
        if (pid > 0)
        {
            // parent
            uber_drivers[i].pid = pid;
            uber_drivers[i].active = 1;
        }
    }
}

void parent_job(mqd_t uber_tasks, uber_driver_t* uber_drivers, int N) {
    // Set the seed for the random number generator
    srand(getpid());

    // Open the message queues for the drivers
    mqd_t* drivers_mq = (mqd_t*) malloc(N * sizeof(mqd_t));
    char name[20];
    for(int i = 0; i < N; i++) {
        sprintf(name, "/uber_results_%d", uber_drivers[i].pid);
        struct mq_attr attr;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = sizeof(uber_task_done_message_t);
        if ((drivers_mq[i] = mq_open(name, O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr)) == (mqd_t) -1) {
            ERR("mq_open");
        }
    }

    while(should_run) {
        // Check for completed rides
        for(int i = 0; i < N; i++) {
            if (uber_drivers[i].active == 0)
                continue;

            uber_task_done_message_t result;
            if (mq_receive(drivers_mq[i], (char*) &result, sizeof(uber_task_done_message_t), NULL) == -1) {
                if (errno == EAGAIN) {
                    continue;
                } else if (errno == ENOENT){
                    uber_drivers[i].active = 0;
                    fprintf(stderr, "Dead driver %d\n", uber_drivers[i].pid);
                    continue;
                } else {
                    ERR("mq_receive");
                }
            }

            printf("Driver %d drove a distance of %d\n", uber_drivers[i].pid, result.driven_distance);
        }


        // Generate a random ride
        uber_task_t ride;
        ride.x_start = rand() % 2000 - 1000;
        ride.y_start = rand() % 2000 - 1000;
        ride.x_end = rand() % 2000 - 1000;
        ride.y_end = rand() % 2000 - 1000;

        if (mq_send(uber_tasks, (char*) &ride, UBER_TASK_MESSAGE_SIZE, 0) == -1) {
            if (errno == EAGAIN) {
                fprintf(stderr, "uber_tasks message queue is full\n");
            } else {
                ERR("mq_send");
            }
        }

        // ysleep for 500 - 2000ms
        msleep(rand() % 1500 + 500);
    }

    // Send special priority end message
    printf("Ending the simulation\n");
    while (children_left > 0) {
        uber_task_t end;
        end.x_start = 0;
        end.y_start = 0;
        end.x_end = 0;
        end.y_end = 0;
        if (mq_send(uber_tasks, (char*) &end, UBER_TASK_MESSAGE_SIZE, ENDING_MESSAGE_PRIORITY) == -1) {
            if (errno != EAGAIN) {
                ERR("mq_send");
            }
        }
    }

    free(drivers_mq);
}

int main(int argc, char** argv)
{
    // Initialize and validate input
    if (argc != 3)
        usage(argv[0]);
    int N, T;
    N = atoi(argv[1]);
    T = atoi(argv[2]);
    if ( N < 1 || T < 5)
        usage(argv[0]);

    // Set appropriate signal handlers
    children_left = N;
    set_handler(alarm_handler, SIGALRM);
    set_handler(sigchld_handler, SIGCHLD);

    // Create uber_tasks_mq message queue
    mqd_t uber_tasks_mq;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = UBER_TASK_MESSAGE_SIZE;
    if ((uber_tasks_mq = mq_open("/uber_tasks", O_WRONLY | O_CREAT | O_NONBLOCK, 0600, &attr)) == (mqd_t) -1)
        ERR("mq_open");

    uber_driver_t* uber_drivers = (uber_driver_t*) malloc(N * sizeof(uber_driver_t));

    // Create the children
    create_children(N, uber_drivers);

    // Set the alarm
    alarm(T);

    // Parent job
    parent_job(uber_tasks_mq, uber_drivers, N);

    // Cleanup
    if (mq_close(uber_tasks_mq) == -1)
        ERR("mq_close");
    if (mq_unlink("/uber_tasks") == -1)
        ERR("mq_unlink");

    // Generate uber tasks
    free(uber_drivers);

    printf("Exiting\n");

    return EXIT_SUCCESS;
}
