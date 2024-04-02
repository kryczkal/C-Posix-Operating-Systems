//
// Created by wookie on 4/2/24.
//
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>

#define MAYBE_UNUSED(x) ((void)(x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define N_ITERATIONS 1000000
#define LOG_LEN 8


void child_work(int n, double* data, char* log){
    int iteration_count = N_ITERATIONS;

    srand(getpid());
    // Monte Carlo method
    int count = 0;
    double x, y;
    for (int i = 0; i < iteration_count; i++) {
        x = (double) rand() / RAND_MAX;
        y = (double) rand() / RAND_MAX;
        if (x * x + y * y <= 1) {
            count++;
        }
    }
    double pi = 4 * (double) count / iteration_count;

    // Write the result to the shared memory
    data[n] = pi;

    // Write the result to the log file
    char log_entry[LOG_LEN + 1];
    snprintf(log_entry, LOG_LEN + 1, "%7.5f\n", pi);
    memcpy(log + n * LOG_LEN, log_entry, LOG_LEN);
}

void parent_work(int n, double* data)
{
    // Wait for all children to finish
    pid_t pid;
    for (;;)
    {
        pid = wait(NULL);
        if (pid <= 0)
        {
            if (errno == ECHILD)
                break;
            ERR("waitpid");
        }
    }

    // Calculate the average of the data
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += data[i];
    sum = sum / n;

    printf("Pi is approximately %f\n", sum);
}

void create_children(int n, double* data, char* log)
{
    while (n-- > 0)
    {
        switch (fork())
        {
            case 0:
                child_work(n, data, log);
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    // Input validation
    if(argc != 2)
        ERR("Usage: ./monte-carlo-pi <number-of-children>");
    int n = atoi(argv[1]);
    if(n <= 0)
        ERR("Invalid number of children");

    /*
     * Create shared memory
     */

    // For the .log file

    // Create the file
    int log_fd;
    if((log_fd = open("./log.txt", O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1)
        ERR("open");
    if(ftruncate(log_fd, n * LOG_LEN) == -1)
        ERR("ftruncate");

    // Map the file
    char* log;
    if((log = mmap(NULL, n * LOG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, log_fd, 0)) == MAP_FAILED)
        ERR("mmap");
    // Close the file descriptor
    if(close(log_fd) == -1)
        ERR("close");

    // For the data
    double* data;
    if((data = mmap(NULL, n * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
        ERR("mmap");

    create_children(n, data, log);
    parent_work(n, data);

    // Cleanup

    if(munmap(data, n * sizeof(double)) == -1)
        ERR("munmap");
    if(msync(log, n * LOG_LEN, MS_SYNC) == -1)
        ERR("msync");
    if(munmap(log, n * LOG_LEN) == -1)
        ERR("munmap");

    return EXIT_SUCCESS;
}
