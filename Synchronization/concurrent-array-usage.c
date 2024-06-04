#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

//#define NON_DYNAMIC_MUTEX

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define ARR_MAX_SIZE 256
#define ARR_MIN_SIZE 8
#define MIN_THREADS 1
#define MAX_THREADS 16
#define UNUSED(x) (void)(x)
#define TRUE 1
#define FALSE 0
#define MS_TO_NSEC (long)(1e+6)

typedef enum { FREE_SLOT, WORKING, STOPPED } threadState_t;

typedef struct ThreadInfo{
    int Array[ARR_MAX_SIZE];

#ifdef NON_DYNAMIC_MUTEX
    pthread_mutex_t guard;
#else
    pthread_mutex_t Guards[ARR_MAX_SIZE];
#endif

    int n;
}ThreadInfo_t;

typedef struct ThreadArgument{
    ThreadInfo_t* jobInfo;
    threadState_t* statusInfo;
    size_t ID;
}ThreadArgument_t;

typedef struct ControlInfo{
    ThreadInfo_t tInfo;
    ThreadArgument_t tArgs[MAX_THREADS];
    pthread_t threads[MAX_THREADS];
    threadState_t threadStatus[MAX_THREADS];
    int p;
}ControlInfo_t;

void usage(const char *pname);
void readArgs(int argc, char** argv, ControlInfo_t* info);
void initInfo(ControlInfo_t* info);
void blockSignals(sigset_t* mask, sigset_t* oMask);
char isFreeSlot(ControlInfo_t* info);
size_t findFreeSlot(ControlInfo_t* info);
void cleanSlots(ControlInfo_t* info);
void processMessage(int sigNo, ControlInfo_t* info);
void* printJob(void* arg);
void* arrayJob(void* arg);

int main(int argc, char *argv[])
{
#ifdef NON_DYNAMIC_MUTEX
    ControlInfo_t mainData = { .tInfo = { .guard = PTHREAD_MUTEX_INITIALIZER } };
#else
    ControlInfo_t mainData;
#endif

    sigset_t mask, oMask;

    srand(time(NULL));
    readArgs(argc, argv, &mainData);
    initInfo(&mainData);
    blockSignals(&mask, &oMask);

    while(TRUE){
        int sigNo;

        sigwait(&mask, &sigNo);
        if (!isFreeSlot(&mainData)){
            printf("All thread busy, aborting request!\n");
        }
        else{
            processMessage(sigNo, &mainData);
            cleanSlots(&mainData);
        }
    }

    exit(EXIT_SUCCESS);
}

void usage(const char *pname) {
    fprintf(stderr, "USAGE: %s n p\n", pname);
    exit(EXIT_FAILURE);
}

void readArgs(int argc, char **argv, ControlInfo_t *info) {
    int n, p;
    if (argc != 3) usage(argv[0]);

    n = atoi(argv[1]);
    p = atoi(argv[2]);

    if (n < ARR_MIN_SIZE || n > ARR_MAX_SIZE || p < MIN_THREADS || p > MAX_THREADS) usage(argv[0]);
    info->p = p;
    info->tInfo.n = n;
}

void initInfo(ControlInfo_t *info) {
    for(size_t i = 0; i < info->p; ++i) {
        info->threadStatus[i] = FREE_SLOT;
        info->tArgs[i].ID = i;
        info->tArgs[i].jobInfo = &info->tInfo;
        info->tArgs[i].statusInfo = info->threadStatus;
    }

    for(int i = 0; i < info->tInfo.n; ++i) {
        info->tInfo.Array[i] = i;

#ifndef NON_DYNAMIC_MUTEX
        if (pthread_mutex_init(&info->tInfo.Guards[i], NULL))
            ERR("Not able to initialize mutex!");
#endif
    }
}

void blockSignals(sigset_t *mask, sigset_t *oMask) {
    sigemptyset(mask);
    sigaddset(mask, SIGUSR1);
    sigaddset(mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, mask, oMask);
}

char isFreeSlot(ControlInfo_t *info) {
    for (size_t i = 0; i < info->p; ++i)
        if (info->threadStatus[i] != WORKING) return TRUE;

    return FALSE;
}

size_t findFreeSlot(ControlInfo_t *info) {
    for (size_t i = 0; i < info->p; ++i){
        if (info->threadStatus[i] != WORKING) return i;
    }

    return (size_t)-1;
}

void processMessage(int sigNo, ControlInfo_t *info) {
    size_t index = findFreeSlot(info);
    info->threadStatus[index] = WORKING;

    switch (sigNo) {
        case SIGUSR1:
            if (pthread_create(&info->threads[index], NULL, arrayJob, &info->tArgs[index]))
                ERR("Not able to create thread");
            break;
        case SIGUSR2:
            if (pthread_create(&info->threads[index], NULL, printJob, &info->tArgs[index]))
                ERR("Not able to create thread");
            break;
        default:
            ERR("Retrieved unexpected signal!");
    }
}

void cleanSlots(ControlInfo_t *info) {
    for (size_t i = 0; i < info->p; ++i){
        if (info->threadStatus[i] == STOPPED)
            if (pthread_join(info->threads[i], NULL))
                ERR("Not able to join thread!");
    }
}

void *arrayJob(void *arg) {
    ThreadArgument_t* tArg = arg;
    ThreadInfo_t* jobParams = tArg->jobInfo;

    int a = rand() % jobParams->n;
    int range = jobParams->n - a;
    int b = a + rand() % range;

    struct timespec ts = { .tv_sec = 0, .tv_nsec = 5 * MS_TO_NSEC };
    while(a < b){

#ifdef NON_DYNAMIC_MUTEX
        pthread_mutex_lock(&jobParams->guard);
#else
        pthread_mutex_lock(jobParams->Guards + a);
        pthread_mutex_lock(jobParams->Guards + b);
#endif
        int temp = jobParams->Array[a];
        jobParams->Array[a] = jobParams->Array[b];;
        jobParams->Array[b] = temp;

#ifdef NON_DYNAMIC_MUTEX
        pthread_mutex_unlock(&jobParams->guard);
#else
        pthread_mutex_unlock(jobParams->Guards + b);
        pthread_mutex_unlock(jobParams->Guards + a);
#endif

        nanosleep(&ts, &ts);
        ++a;
        --b;
    }

    tArg->statusInfo[tArg->ID] = STOPPED;
    return NULL;
}

void *printJob(void *arg) {
    ThreadArgument_t* tArg = arg;
    ThreadInfo_t* jobParams = tArg->jobInfo;

#ifdef NON_DYNAMIC_MUTEX
    pthread_mutex_lock(&jobParams->guard);
#else
    for (size_t i = 0; i < jobParams->n; ++i){
        pthread_mutex_lock(jobParams->Guards + i);
    }
#endif

    printf("Printing array:\n[ ");
    for (int i = 0; i < jobParams->n; ++i){
        printf("%d ", jobParams->Array[i]);
    }
    printf("]\n");

#ifdef NON_DYNAMIC_MUTEX
    pthread_mutex_unlock(&jobParams->guard);
#else
    for (size_t i = 0; i < jobParams->n; ++i){
        pthread_mutex_unlock(jobParams->Guards + i);
    }
#endif

    tArg->statusInfo[tArg->ID] = STOPPED;
    return NULL;
}
