/* This C program simulates a dice game with multiple players (threads), each rolling a die in rounds. 
 * Each player rolls a die and the player(s) with the highest roll in each round receive a point. 
 * The game continues for a predefined number of rounds, and at the end, the points are tallied to determine the winner(s).
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define ERR(str) (fprintf(stderr, "on line: %d, in file: %s\n", __LINE__, __FILE__), perror(str), exit(EXIT_FAILURE))

#define PLAYERS 4
#define ROUNDS 10

typedef struct thread_arg {
    char id;
    unsigned seed;
    char* rolls;
    char* points;
    pthread_barrier_t* barrier;
}thread_arg_t;

typedef struct thread_info {
    pthread_t tid[PLAYERS];
    char rolls[PLAYERS];
    char points[PLAYERS];
    thread_arg_t args[PLAYERS];
    pthread_barrier_t barrier;
}thread_info_t;

void* thread_job(void* vArg) {
    thread_arg_t* arg = vArg;

    for (int round = 0 ; round < ROUNDS; ++round) {
        arg->rolls[arg->id] = 1 + rand_r(&arg->seed) % 6;

        if (PTHREAD_BARRIER_SERIAL_THREAD == pthread_barrier_wait(arg->barrier)) {
            char maxScore = -1;

            // Finding highest score
            for (size_t i = 0 ; i < PLAYERS; ++i)
                if (arg->rolls[i] > maxScore)
                    maxScore = arg->rolls[i];

            // Assigning to all players with highest score!
            for (size_t i = 0; i < PLAYERS; ++i)
                if (arg->rolls[i] == maxScore)
                    arg->points[i]++;
        }

        pthread_barrier_wait(arg->barrier);
    }

    return NULL;
}

void init_args(thread_info_t* tInfo) {
    srand(time(NULL));
    if (pthread_barrier_init(&tInfo->barrier, NULL, PLAYERS))
        ERR("failed when creating barrier!");

    for (size_t i = 0; i < PLAYERS; ++i) {
        tInfo->points[i] = 0;
        tInfo->args[i].barrier = &tInfo->barrier;
        tInfo->args[i].points = tInfo->points;
        tInfo->args[i].rolls = tInfo->rolls;
        tInfo->args[i].id = i;
        tInfo->args[i].seed = rand();
    }
}

void create_threads(thread_info_t* tInfo) {
    for (size_t i = 0; i < PLAYERS; ++i) {
        if (pthread_create(tInfo->tid + i, NULL, thread_job, tInfo->args + i))
            ERR("Not able to creat workers");
    }
}

void cleanup(thread_info_t* tInfo) {
    for (size_t i = 0 ; i < PLAYERS; ++i)
        pthread_join(tInfo->tid[i], NULL);

    pthread_barrier_destroy(&tInfo->barrier);
}

void writeResult(const thread_info_t* const tInfo) {
    printf("Final scores: \n");
    for (size_t i = 0; i < PLAYERS; ++i) {
        printf("Player %ld, acquired result: %d\n", i, tInfo->points[i]);
    }
}

int main() {
    thread_info_t tInfo;

    init_args(&tInfo);
    create_threads(&tInfo);
    cleanup(&tInfo);
    writeResult(&tInfo);

    return EXIT_SUCCESS;
}