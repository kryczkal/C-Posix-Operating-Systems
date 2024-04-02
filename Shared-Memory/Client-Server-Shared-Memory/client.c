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

void usage(char *name) {
    fprintf(stderr, "USAGE: %s <pid>\n", name);
    fprintf(stderr, "pid - pid of the server\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char* argv[]) {
    /*
     * Input validation
     */

    if(argc != 2)
        usage(argv[0]);
    pid_t server_pid = atoi(argv[1]);
    if(server_pid <= 0)
        ERR("Invalid pid");

    /*
     * Shared memory opening
     */

    int shm_fd;
    char shm_name[32];
    sprintf(shm_name, "/%d-board", server_pid);
    if((shm_fd = shm_open(shm_name, O_RDWR, 0)) == -1)
        ERR("shm_open");

    /*
     * Shared memory mapping
     */

    int *grid;
    if((grid = (int*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    // Close the shared memory file descriptor
    if(close(shm_fd) == -1)
        ERR("close");
    // Get the shared mutex
    pthread_mutex_t *shared_mutex = (pthread_mutex_t*)grid;
    char* n_shared = (char *) grid + sizeof(pthread_mutex_t);
    int n = *n_shared;
    char* grid_start = n_shared + sizeof(char);
    /*
     * Client work
     */
    srand(getpid());

    int score = 0;
    int should_run = 1;
    while(should_run) {
        // Lock the shared mutex
        int status;
        if( (status = pthread_mutex_lock(shared_mutex)) != 0) {
            if(status == EOWNERDEAD){
                pthread_mutex_consistent(shared_mutex);
                printf("Recovering from a disconnected client\n");
            }else{
                ERR("pthread_mutex_lock");
            }
        }

        // Randomly disconnect the client
        int D = 1 + rand() % 9;
        if(D == 1) {
            printf("Client %d disconnected\n", getpid());
            exit(EXIT_SUCCESS);
        }

        // Make a random move
        int x = rand() % n;
        int y = rand() % n;

        printf("Client %d tries to search field (%d, %d)\n", getpid(), x, y);
        int num = grid_start[x * n + y];
        grid_start[x * n + y] = 0; // Mark the field as visited
        score += num;
        printf("Client %d found %d at (%d, %d)\n", getpid(), num, x, y);
        // Disconnect the client if the number is 0 else add the number to the score
        if(num == 0) {
            printf("Client %d game over\n", getpid());
            printf("Client %d score: %d\n", getpid(), score);
            should_run = 0;
        }

        // Unlock the shared mutex
        if (pthread_mutex_unlock(shared_mutex)) ERR("pthread_mutex_unlock");

        for(int i = sleep(1); i > 0; i = sleep(i)); // Sleep for 1 second
    }

    /*
     * Cleanup
     */

    if(munmap(grid, SHM_SIZE) == -1)
        ERR("munmap");
    exit(EXIT_SUCCESS);
}
