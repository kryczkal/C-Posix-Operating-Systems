//
// Created by kryczkal on 3/6/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define ERR(source) (perror(source),\
             fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
             exit(EXIT_FAILURE))

void usage(char *name) {
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "n - number of processes\n");
    exit(EXIT_FAILURE);
}

int** create_pipe_cycle(int n) {
    return (int**) malloc(n * sizeof(int*));
}

void initialize_pipe_cycle(int** pipe_cycle, int n) {
    for(int i = 0; i < n; i++){
        pipe_cycle[i]= (int*) malloc(2 * sizeof(int));
        if(pipe(pipe_cycle[i])) ERR("initialize pipe");
    }
}

void free_pipe_cycle(int** pipe_cycle, int n){
    for(int i = 0; i < n; i++){
        if (close(pipe_cycle[i][0]) && errno != EBADF) { ERR("closing pipe"); }
        if (close(pipe_cycle[i][1]) && errno != EBADF) { ERR("closing pipe"); }
        free(pipe_cycle[i]);
    }
    free(pipe_cycle);
}

void print_pipe_cycle(int** pipe_cycle, int n){
    for(int i = 0; i < n; i++){
        printf("%d, %d\n", pipe_cycle[i][0], pipe_cycle[i][1]);
    }
}

int main(int argc, char** argv) {
    int n;
    if (argc != 2) usage(argv[0]);
    if ((n = atoi(argv[1])) < 1) usage(argv[0]);
    int** pipe_cycle = create_pipe_cycle(n);
    initialize_pipe_cycle(pipe_cycle, n);
    print_pipe_cycle(pipe_cycle, n);
    free_pipe_cycle(pipe_cycle, n);
}