/* The program operates in a loop for a defined duration (SIMULATION_TIME_MS), periodically removing students at random and
 * adjusting the active student count. It concludes by joining all threads to ensure the program cleanly exits, summarizing
 * the final distribution of students across the years.
 */


#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "In file: %s, on line: %d", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define DEFAULT_STUDENT_COUNT 90
#define SIMULATION_TIME_MS 4000
#define FIRST_YEAR 0
#define YEAR_AMOUNT 4
#define TRUE 1
#define MSEC_TO_NSEC 1000
#define FALSE 0

typedef unsigned long UL;

typedef struct YearCounters{
    int counter[YEAR_AMOUNT];
    pthread_mutex_t guards[YEAR_AMOUNT];
}YearCounters_t;

typedef struct removeData{
    size_t actualYear;
    YearCounters_t* yrs;
}RemoveData_t;

typedef struct StudentInfo{
    char* isAlive;
    pthread_t* students;
    size_t studentCount;
    size_t activeStudents;
}StudentInfo_t;

void Usage(char* progName);
int ReadArgs(int argc, char** argv);
void* StudentJob(void* args);
StudentInfo_t PerformStudentInit(int argc, char** argv , YearCounters_t* resources);
void RemoveActiveStudent(StudentInfo_t* info, size_t studentIndex);
void mSleep(UL mSec);
void RemoveSelf(void* data);

int main(int argc, char** argv){
    srand(time(NULL));
    YearCounters_t yCounters = {
            .counter = { 0, 0, 0, 0 },
            .guards = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
                        PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER}
            };
    StudentInfo_t students = PerformStudentInit(argc, argv, &yCounters);

    UL spentTime = 0;
    while(spentTime < SIMULATION_TIME_MS){
        UL sleepTime = 100 + rand() % 201;
        spentTime += sleepTime;

        mSleep(sleepTime);
        size_t studentToRemoveIndex = rand() % students.activeStudents;
        RemoveActiveStudent(&students, studentToRemoveIndex);
    }

    for(size_t i = 0; i < students.studentCount; ++i){
        if (pthread_join(students.students[i], NULL)){
            ERR("Error when joining some student!");
        }
    }

    printf(" First year: %d\n", yCounters.counter[0]);
    printf("Second year: %d\n", yCounters.counter[1]);
    printf(" Third year: %d\n", yCounters.counter[2]);
    printf("  Engineers: %d\n", yCounters.counter[3]);

    free(students.isAlive);
    free(students.students);
    return EXIT_SUCCESS;
}

void RemoveActiveStudent(StudentInfo_t* info, size_t studentIndex){
    size_t index = 0;

    for (size_t i = 0; index != studentIndex; ++i){
        if (info->isAlive[i] == TRUE) ++index;
    }

    pthread_cancel(info->students[index]);
    info->isAlive[index] = FALSE;
    info->activeStudents--;
}

StudentInfo_t PerformStudentInit(int argc, char** argv, YearCounters_t* resources){
    StudentInfo_t ret;
    ret.studentCount = ReadArgs(argc, argv);
    ret.activeStudents = ret.studentCount;

    ret.isAlive = (char*)malloc(sizeof(char)*ret.studentCount);
    if (ret.isAlive == NULL){
        ERR("Not able to allocate students resources!");
    }

    ret.students = (pthread_t*)malloc(sizeof(pthread_t)*ret.studentCount);
    if (ret.students == NULL){
        ERR("Not able to allocate students resources!");
    }

    for (size_t i = 0; i < ret.studentCount; ++i){
        ret.isAlive[i] = TRUE;
        if (pthread_create(&ret.students[i], NULL, StudentJob, resources))
            ERR("Some student wasn't able to begin his university year!");
    }

    return ret;
}

void RemoveSelf(void* data){
    RemoveData_t* rData = data;

    pthread_mutex_lock(rData->yrs->guards + rData->actualYear);
    rData->yrs->counter[rData->actualYear] -= 1;
    pthread_mutex_unlock(rData->yrs->guards + rData->actualYear);
}

void* StudentJob(void* args){
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    YearCounters_t* yCounters = args;

    pthread_mutex_lock(yCounters->guards);
    ++yCounters->counter[FIRST_YEAR];
    pthread_mutex_unlock(yCounters->guards);

    RemoveData_t remData = {.yrs = yCounters};
    for (int yearEnded = 1; yearEnded < YEAR_AMOUNT; ++yearEnded){

        remData.actualYear = yearEnded - 1;
        pthread_cleanup_push(RemoveSelf, &remData);
        struct timespec ts =  {.tv_sec = 1};
        nanosleep(&ts, NULL);
        pthread_cleanup_pop(0);

        // Removing self from the previous year
        pthread_mutex_lock(yCounters->guards + yearEnded-1);
        yCounters->counter[yearEnded-1] -= 1;
        pthread_mutex_unlock(yCounters->guards + yearEnded-1);

        // Adding self to next year
        pthread_mutex_lock(yCounters->guards + yearEnded);
        yCounters->counter[yearEnded] += 1;
        pthread_mutex_unlock(yCounters->guards + yearEnded);
    }

    return NULL;
}

int ReadArgs(int argc, char** argv){
    int studentsCount;

    switch (argc) {
        case 1:
            return DEFAULT_STUDENT_COUNT;
        case 2:
            studentsCount = atoi(argv[1]);
            if (studentsCount <= 0)
                Usage(argv[0]);
            return studentsCount;
        default:
            Usage(argv[0]);
    }

    return  0; // dead branch
}

void mSleep(UL mSec){
    struct timespec ts = { .tv_sec = 0, .tv_nsec = mSec * MSEC_TO_NSEC};

    if (nanosleep(&ts, NULL)){
        ERR("nanosleep");
    }
}

void Usage(char* progName){
    fprintf(stderr, "%s [ optional: n ]\nWhere n is number of students, n <= 100, by the default n = 90\n", progName);
    fprintf(stderr, "Program simulates student's situation at MiNI faculty.\n");
    exit(EXIT_SUCCESS);
}