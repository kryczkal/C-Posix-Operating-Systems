/* This C program simulates a card game with a variable number of players, demonstrating:
 * - Thread creation and management: Uses pthreads to represent players participating in the game.
 * - Synchronization mechanisms: Employs mutexes, condition variables, and barriers to synchronize actions among threads, 
 *   ensuring orderly gameplay and handling of game events (e.g., player joining, card exchange).
 * - Signal handling: Utilizes signals (SIGUSR1 to add a player, SIGINT to terminate the game) to interact with the game 
 *   dynamically, showcasing how external events can control thread behavior.
 * - Randomization: Shuffles the deck and determines the gameplay flow, such as which cards are exchanged, to mimic the 
 *   unpredictability of a real card game.
 * - Resource cleanup: Carefully cleans up resources (threads, mutexes, condition variables, and barriers) upon game completion 
 *   or interruption, demonstrating proper resource management in a multithreaded environment.
 * - Game logic implementation: Incorporates game-specific logic, such as checking for winning conditions (all cards of the same 
 *   suit) and managing the deck and hands of cards.
 *
 * Players (threads) each receive a hand of cards and participate in rounds of the game where they may exchange a card with 
 * the next player. The game continues until a player wins by collecting a hand of the same suit or the game is interrupted 
 * using SIGINT. This program illustrates complex thread interaction and synchronization in a simulated environment.
 */


#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define UNUSED(x) ((void)(x))

#define DECK_SIZE (4 * 13)
#define HAND_SIZE (7)
#define MIN_PLAYERS 4
#define MAX_PLAYERS 7

void shuffle(int *array, size_t n)
{
    if (n > 1)
    {
        for (size_t i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void print_deck(const int *deck, int size)
{
    const char *suits[] = {" of Hearts", " of Diamonds", " of Clubs", " of Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    char buffer[1024];
    int offset = 0;

    if (size < 1 || size > DECK_SIZE)
        return;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[");
    for (int i = 0; i < size; ++i)
    {
        int card = deck[i];
        if (card < 0 || card > DECK_SIZE)
            return;
        int suit = deck[i] % 4;
        int value = deck[i] / 4;

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", values[value]);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", suits[suit]);
        if (i < size - 1)
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "]");

    puts(buffer);
}

void usage(const char *program_name)
{
    fprintf(stderr, "USAGE: %s n\n", program_name);
    exit(EXIT_FAILURE);
}

typedef struct thread_arg{
    size_t id;
    size_t playerCount;
    size_t* alivePlayers;
    size_t* readyPlayers;
    unsigned seed;
    char* isGameOn;
    char* shouldEnd;
    int* hand;
    int** hands;
    pthread_barrier_t* gameBarrier;
    pthread_mutex_t* startMutex;
    pthread_cond_t* startCV;
}thread_arg_t;

typedef struct thread_info {
    pthread_t tid[MAX_PLAYERS];
    thread_arg_t tArgs[MAX_PLAYERS];
    sigset_t oMask;
    sigset_t mask;
    size_t playersCount;

    int deck[DECK_SIZE];
    int hands[MAX_PLAYERS][HAND_SIZE + 1];
    int* threadHands[MAX_PLAYERS];
    size_t tableSize;
    size_t alivePlayers;
    size_t readyPlayers;
    pthread_barrier_t gameBarrier;
    pthread_cond_t gameStartCV;
    pthread_mutex_t gameStartMutex;
    char isGameOn;
    char shouldEnd;
}thread_info_t;

int read_args(int argc, char** argv) {
    if (argc != 2)
        usage(argv[0]);

    int pCount = atoi(argv[1]);

    if (pCount < MIN_PLAYERS || pCount > MAX_PLAYERS)
        usage(argv[0]);

    return pCount;
}

void init(thread_info_t* tInfo) {
    srand(time(NULL));

    sigemptyset(&tInfo->mask);
    sigaddset(&tInfo->mask, SIGUSR1);
    sigaddset(&tInfo->mask, SIGINT);
    sigprocmask(SIG_BLOCK, &tInfo->mask, &tInfo->oMask);

    for(size_t i = 0; i < DECK_SIZE; ++i)
        tInfo->deck[i] = i;

    tInfo->playersCount = 0;
    tInfo->alivePlayers = 0;
    tInfo->readyPlayers = 0;
    tInfo->shouldEnd = 0;

    for(size_t i = 0; i < MAX_PLAYERS; ++i) {
        tInfo->threadHands[i] = &(tInfo->hands[i][0]);
    }

    for (size_t i = 0; i < tInfo->tableSize; ++i) {
        tInfo->tArgs[i].id = i;
        tInfo->tArgs[i].seed = rand();
        tInfo->tArgs[i].playerCount = tInfo->tableSize;
        tInfo->tArgs[i].alivePlayers = &tInfo->alivePlayers;
        tInfo->tArgs[i].readyPlayers = &tInfo->readyPlayers;
        tInfo->tArgs[i].hand = tInfo->hands[i];
        tInfo->tArgs[i].hands = tInfo->threadHands;
        tInfo->tArgs[i].gameBarrier = &tInfo->gameBarrier;
        tInfo->tArgs[i].isGameOn = &tInfo->isGameOn;
        tInfo->tArgs[i].shouldEnd = &tInfo->shouldEnd;
        tInfo->tArgs[i].startMutex = &tInfo->gameStartMutex;
        tInfo->tArgs[i].startCV = &tInfo->gameStartCV;
    }

    if (pthread_barrier_init(&tInfo->gameBarrier, NULL, tInfo->tableSize))
        ERR("Not able to create barrier");

    if (pthread_mutex_init(&tInfo->gameStartMutex, NULL))
        ERR("Not able to init mutes!");

    if (pthread_cond_init(&tInfo->gameStartCV, NULL))
        ERR("Not able to init conditional variable!");
}

void checkForWin(thread_arg_t* arg) {
    int winCol = arg->hand[0] % 4;

    for(size_t i = 1; i < HAND_SIZE; ++i) {
        if (arg->hand[i] % 4 != winCol)
            return;
    }

    printf("[ PLAYER %lu ] My ship sails!\n", arg->id);
    print_deck(arg->hand, HAND_SIZE);
    *(arg->isGameOn) = 0;
}

int interruptable_Barrier(pthread_cond_t* cv, pthread_mutex_t* mt, size_t* incremental, size_t barrier_size){
    int ret = 0;

    pthread_mutex_lock(mt);
    (*incremental)++;
    if (*incremental == barrier_size) {

        *incremental = 0;
        pthread_cond_broadcast(cv);
        ret = PTHREAD_BARRIER_SERIAL_THREAD;
    }
    else {
        pthread_cond_wait(cv, mt);
    };

    pthread_mutex_unlock(mt);

    return ret;
}

void* player_thread(void* vArg) {
    thread_arg_t* arg = vArg;
    printf("[ PLAYER %lu ] Players's cards:\n", arg->id);
    print_deck(arg->hand, HAND_SIZE);

    pthread_mutex_lock(arg->startMutex);
    (*(arg->alivePlayers))++;
    if (*arg->alivePlayers == arg->playerCount) {
        pthread_cond_broadcast(arg->startCV);
    }
    else {
        pthread_cond_wait(arg->startCV, arg->startMutex);
    };
    pthread_mutex_unlock(arg->startMutex);

    checkForWin(arg);
    interruptable_Barrier(arg->startCV, arg->startMutex, arg->readyPlayers, arg->playerCount);

    while(*(arg->isGameOn) && !*(arg->shouldEnd)) {
        size_t exchangeCard = rand_r(&arg->seed) % HAND_SIZE;
        arg->hand[HAND_SIZE] = arg->hand[exchangeCard];

        for(size_t i = exchangeCard; i < HAND_SIZE; ++i) {
            arg->hand[i] = arg->hand[i + 1];
        }

        if (PTHREAD_BARRIER_SERIAL_THREAD == interruptable_Barrier(arg->startCV, arg->startMutex, arg->readyPlayers, arg->playerCount)) {
            for (size_t i = 0; i < arg->playerCount; ++i) {
                (arg->hands)[i][HAND_SIZE - 1] = (arg->hands)[(i + 1) % arg->playerCount][HAND_SIZE];
            }
        }

        interruptable_Barrier(arg->startCV, arg->startMutex, arg->readyPlayers, arg->playerCount);

        printf("[ PLAYER %lu ] Player's cards:\n", arg->id);
        print_deck(arg->hand, HAND_SIZE);

        checkForWin(arg);
        interruptable_Barrier(arg->startCV, arg->startMutex, arg->readyPlayers, arg->playerCount);
    }

    pthread_mutex_lock(arg->startMutex);
    (*(arg->alivePlayers))--;
    pthread_mutex_unlock(arg->startMutex);

    return NULL;
}

void placePlayerAtTheTable(thread_info_t* tInfo) {
    if (tInfo->playersCount == tInfo->tableSize) {
        printf("[ TABLE ] Table is full!\n");
        if (tInfo->isGameOn == 1) {
            printf("[ TABLE ] There is only room for %lu at the table!\n", tInfo->tableSize);
            return;
        }

        for (size_t i = 0; i < tInfo->tableSize; ++i)
            pthread_join(tInfo->tid[i], NULL);
        tInfo->playersCount = 0;
        tInfo->alivePlayers = 0;
        shuffle(tInfo->deck, DECK_SIZE);

        printf("[ TABLE ] Cleaned table after previous players!\n");
    }

    for (size_t i = 0; i < HAND_SIZE; ++i) {
        (tInfo->hands)[tInfo->playersCount][i] = tInfo->deck[tInfo->playersCount * HAND_SIZE + i];
    }

    printf("[ TABLE ] Placed player %lu at the table!\n", tInfo->playersCount + 1);
    if (pthread_create(&tInfo->tid[tInfo->playersCount], NULL, player_thread, &tInfo->tArgs[tInfo->playersCount]))
        ERR("Not able to create thread!");

    if (++(tInfo->playersCount) == tInfo->tableSize)
        tInfo->isGameOn = 1;
}

void main_job(thread_info_t* tInfo) {
    int sigNo;
    shuffle(tInfo->deck, DECK_SIZE);

    while(!tInfo->shouldEnd) {
        sigwait(&tInfo->mask, &sigNo);

        switch (sigNo) {
            case SIGUSR1:
                placePlayerAtTheTable(tInfo);
                break;
            case SIGINT:
                printf("[ TABLE ] Telling players they have to leave!\n");

                tInfo->shouldEnd = 1;
                while(tInfo->alivePlayers){
                    pthread_mutex_lock(&tInfo->gameStartMutex);
                    pthread_cond_broadcast(&tInfo->gameStartCV);
                    pthread_mutex_unlock(&tInfo->gameStartMutex);
                }
                break;
            default:
                ERR("Received unrecognized signal!");
        }
    }

}

void cleanup(thread_info_t* tInfo) {
    printf("Starting cleanup...\n");

    for (size_t i = 0 ; i < tInfo->playersCount; ++i) {
        pthread_join(tInfo->tid[i], NULL);
    }

    pthread_mutex_destroy(&tInfo->gameStartMutex);
    pthread_cond_destroy(&tInfo->gameStartCV);
    pthread_barrier_destroy(&tInfo->gameBarrier);

    if (sigprocmask(SIG_SETMASK, &tInfo->oMask, NULL))
        ERR("Not able to recover old mask!");

}

int main(int argc, char *argv[])
{
    thread_info_t tInfo;

    tInfo.tableSize = read_args(argc, argv);
    init(&tInfo);
    main_job(&tInfo);
    cleanup(&tInfo);

    exit(EXIT_SUCCESS);
}