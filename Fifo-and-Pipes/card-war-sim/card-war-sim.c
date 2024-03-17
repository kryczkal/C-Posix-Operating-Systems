//
// Created by kryczkal on 3/6/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define ERR(source) (perror(source),\
             fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
             exit(EXIT_FAILURE))

#define MSG_LENGTH 16 // in bytes
#define MSG_START_ROUND "new_round"
#define CLOSED_PIPE -1

void usage(char *name);
void parse_args(int argc, char *const *argv, int *n, int *m);
void create_children(int n, int** pipes_to_children, int* pipe_to_parent);
void child_work(int pipe_read, int pipe_write);


int **initialize_pipes_to_children(int n);
void free_pipes_to_children(int n, int **pipes_to_children);

typedef struct player_data {
    int score;
    int write_pipe;
} player_data;

typedef struct node {
    player_data data;
    struct node* next;
} node;

int main(int argc, char** argv) {
    int n;
    int m;
    parse_args(argc, argv, &n, &m);

    int** pipes_to_children = initialize_pipes_to_children(n);
    int pipe_to_parent[2];
    if(pipe(pipe_to_parent) == -1) ERR("pipe");
    create_children(n, pipes_to_children, pipe_to_parent);

    free_pipes_to_children(n, pipes_to_children);
}


void child_work(int pipe_read, int pipe_write) {


    if(close(pipe_read) == -1) ERR("close");
    if(close(pipe_write) == -1) ERR("close");
}

void parent_work(int reciever_pipe, int** pipes_to_children, int n, int m) {
    int player_count = n;

    char message[MSG_LENGTH];
    for(int i = 0; i < m; i++){
        // announce start of the round;
        strcpy(message, MSG_START_ROUND);
        memset(message + strlen(MSG_START_ROUND), 0, MSG_LENGTH - strlen(MSG_START_ROUND));

        fprintf(stderr, "Round: %d\n", i);

        for(int j = 0; j < n; j++) {
            if(pipes_to_children[j][1] == CLOSED_PIPE) continue;
            if (write(pipes_to_children[j][1], message, MSG_LENGTH) == -1) {
                if (errno != EPIPE) ERR("write");
                else {
                    if(close(pipes_to_children[j][1]) == -1) ERR("close"); // close the pipe
                    pipes_to_children[j][1] = CLOSED_PIPE; // eliminate the player from the game
                    player_count--;
                }
            }
        }

        if (player_count == 0) return; // all players have been eliminated

        // read the results of the round
        int card = 0;
        int strongest_card = 0;
        int winner = 0;
        for(int j = 0; j < n; j++) {
            if (read(reciever_pipe, message, MSG_LENGTH) == -1) {
                if (errno != EPIPE) ERR("read");
                else if (player_count > 0) {
                    ERR("players should not be eliminated yet");
                }
                card = (int)(message);
                if(card > strongest_card) {
                    strongest_card = card;
                    winner = j;
                }
            }

        }
    }
}

void free_pipes_to_children(int n, int **pipes_to_children) {
    for(int i = 0; i < n; i++) {
        if(close(pipes_to_children[i][0]) == -1 && errno != EBADF) ERR("close");
        if(close(pipes_to_children[i][1]) == -1 && errno != EBADF) ERR("close");
        free(pipes_to_children[i]);
    }
    free(pipes_to_children);
}

int **initialize_pipes_to_children(int n) {
    int** pipes_to_children = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        pipes_to_children[i] = malloc(2 * sizeof(int));
        if(pipe(pipes_to_children[i]) == -1) ERR("pipe");
    }
    return pipes_to_children;
}

void usage(char *name) {
    fprintf(stderr, "USAGE: %s N M\n", name);
    fprintf(stderr, "N - number of players, 2 <= N <= 5\n");
    fprintf(stderr, "M - number of cards, 5 <= M <= 10\n");
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char *const *argv, int *n, int *m) {
    (*n) = atoi(argv[1]);
    (*m) = atoi(argv[2]);
    if (argc != 3) usage(argv[0]);
    if((*n) < 2 || (*n) > 5 || (*m) < 5 || (*m) > 10) usage(argv[0]);
}

void create_children(int n, int **pipes_to_children, int *pipe_to_parent) {
    for(int i = 0; i < n; i++) {
        switch(fork()) {
            case 0:
                // child
                // read from pipes_to_children[i][0] (unique for each child) and write to pipe_to_parent[1] (common for all children)
                child_work(pipes_to_children[i][0], pipe_to_parent[1]);
                return;
            case -1:
                ERR("fork");
            default:
                // parent
                break;
        }
    }
}
