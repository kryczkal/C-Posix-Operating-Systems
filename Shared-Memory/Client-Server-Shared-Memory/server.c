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
#include <pthread.h>

#define MAYBE_UNUSED(x) ((void)(x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_SIZE 4096

typedef struct
{
    int running;
    pthread_mutex_t mutex;
    sigset_t old_mask, new_mask;
} sighandling_args_t;

void* sighandling(void* args)
{
    sighandling_args_t* sighandling_args = (sighandling_args_t*)args;
    int signo;
    if (sigwait(&sighandling_args->new_mask, &signo))
        ERR("sigwait failed.");
    if (signo != SIGINT)
    {
        ERR("unexpected signal");
    }

    pthread_mutex_lock(&sighandling_args->mutex);
    sighandling_args->running = 0;
    pthread_mutex_unlock(&sighandling_args->mutex);
    return NULL;
}
void usage(char *name) {
    fprintf(stderr, "USAGE: %s <n>\n", name);
    fprintf(stderr, "n - size of the grid\n");
    exit(EXIT_FAILURE);
}

void parent_work(int n, char *grid, sighandling_args_t *sighandling_args, pthread_mutex_t *shared_mutex) {
    while(1){
        // Check if the server is still running
        pthread_mutex_lock(&sighandling_args->mutex);
        if(!sighandling_args->running)
            break;
        pthread_mutex_unlock(&sighandling_args->mutex);

        // Handle disconnected clients
        int status;
        if( (status = pthread_mutex_lock(shared_mutex)) != 0) {
            if(status == EOWNERDEAD){
                pthread_mutex_consistent(shared_mutex);
                printf("Recovering from a disconnected client\n");
            }else{
                ERR("pthread_mutex_lock");
            }
        }

        // Print the grid
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
                printf("%d ", grid[i * n + j]);
            printf("\n");
        }
        printf("\n");

        // Unlock the mutex
        if(pthread_mutex_unlock(shared_mutex) != 0)
            ERR("pthread_mutex_unlock");

        // Sleep for 3 seconds
        for(int i = sleep(3); i > 0; i = sleep(i));
    }
}

int main(int argc, char *argv[]) {
    /*
     * Arguments
     */

    // Validate arguments
    if(argc != 2)
        usage(argv[0]);
    int n = atoi(argv[1]);
    if(n <= 0)
        usage(argv[0]);

    /*
     * Initialization
     */

    // Broadcast the server PID to the clients
    const pid_t pid = getpid();
    printf("Server PID: %d\n", pid);

    // Seed the random number generator (for the grid initialization)
    srand(pid);

    /*
     * Shared memory
     */

    // Create shared memory
    char shm_name[32];
    snprintf(shm_name, 32, "/%d-board", pid);
    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0644);
    if(shm_fd == -1)
        ERR("shm_open");

    int grid_size = n * n;
    if(ftruncate(shm_fd, SHM_SIZE) == -1)
        ERR("ftruncate");

    // Map shared memory
    int *grid = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(grid == MAP_FAILED)
        ERR("mmap");

    // Close the file descriptor
    if(close(shm_fd) == -1)
        ERR("close");

    /*
     * Shared mutex
     */

    // Set the shared_mutex attr to PTHREAD_PROCESS_SHARED and PTHREAD_MUTEX_ROBUST
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr) != 0)
        ERR("pthread_mutexattr_init");
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        ERR("pthread_mutexattr_setpshared");
    if(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != 0)
        ERR("pthread_mutexattr_setrobust");

    // Initialize shared_mutex
    pthread_mutex_t *shared_mutex = (pthread_mutex_t *) grid;
    if(pthread_mutex_init(shared_mutex, &attr) != 0)
        ERR("pthread_mutex_init");

    /*
     * Grid
     */

    // Initialize the grid
    char* n_shared = (char *) grid + sizeof(pthread_mutex_t);
    n_shared[0] = n;
    char* grid_start = n_shared + sizeof(char);

    for(int i = 0; i < grid_size; i++)
        grid_start[i] = rand() % 9 + 1;

    /*
     * Signal handling thread
     */

    // Initialize sighandling thread, and its args
    sighandling_args_t sighandling_args = {1, PTHREAD_MUTEX_INITIALIZER, {0}, {0}};
    sigemptyset(&sighandling_args.new_mask);
    sigaddset(&sighandling_args.new_mask, SIGINT);
    // Block SIGINT in the main thread
    if(pthread_sigmask(SIG_BLOCK, &sighandling_args.new_mask, &sighandling_args.old_mask) != 0)
        ERR("pthread_sigmask");

    pthread_t sighandling_thread;
    if(pthread_create(&sighandling_thread, NULL, sighandling, &sighandling_args) != 0)
        ERR("pthread_create");
    /*
     * Main loop
     */

    parent_work(n, grid_start, &sighandling_args, shared_mutex);

    /*
     * Cleanup
     */

    // Join the sighandling thread
    if(pthread_join(sighandling_thread, NULL) != 0)
        ERR("pthread_join");
    // Destroy the shared mutex
    if(pthread_mutex_destroy(shared_mutex) != 0)
        ERR("pthread_mutex_destroy");
    // Destroy the shared mutex attr
    if(pthread_mutexattr_destroy(&attr) != 0)
        ERR("pthread_mutexattr_destroy");
    // Unmap the shared memory
    if(munmap(grid, SHM_SIZE) == -1)
        ERR("munmap");
    // Unlink the shared memory
    if(shm_unlink(shm_name) == -1)
        ERR("shm_unlink");
}
