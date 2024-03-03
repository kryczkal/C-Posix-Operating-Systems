/*
 * This program simulates a classroom scenario where a teacher assigns tasks to students, and students work on these tasks in parts, potentially encountering issues that delay their progress.
 * The program requires three or more arguments: the number of task parts (1-10), the time needed to complete a single task part (1-10), and a list of probabilities (0-100) representing the chance each student might need additional time on a task part.
 * 
 * Each student process represents a student working on the task. If a student encounters an issue (based on their issue probability), it takes additional time to complete the current task part. After finishing each part, the student signals the teacher for approval before proceeding.
 * The teacher process waits for students to signal completion of each part, upon which it sends back approval for the student to continue. This interaction is facilitated through SIGUSR1 and SIGUSR2 signals.
 * 
 * The program uses advanced signal handling to manage communication between the teacher and students. SIGUSR1 is used by students to notify the teacher of task part completion, and SIGUSR2 is used by the teacher to approve continuation.
 * Upon completion of all tasks, each student exits with a status code representing the number of issues they encountered. The teacher collects these exit statuses to determine the total number of issues faced by students.
 * 
 * This program demonstrates process creation and management, signal handling, dynamic memory allocation, and inter-process communication (IPC) in a simulated real-world scenario.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define ERR(str) (fprintf(stderr, "Error on line: %d in file: %s\n", __LINE__, __FILE__), perror(str), kill(0, SIGKILL) \
                ,exit(EXIT_FAILURE))

#define TRUE 1
#define FALSE 0

 volatile sig_atomic_t isApproved = FALSE;
 volatile sig_atomic_t studentsPID = FALSE;
// volatile sig_atomic_t isChildDead = 0;

void usage(char** argv){
    fprintf(stderr, "%s taskParts taskTime [ list of students issue probability ]\n", argv[0]);
    fprintf(stderr, "Where 1 <= taskParts <= 10 is necessary argument, how many parts to complete task\n");
    fprintf(stderr, "Where 1 <= taskTime <= 10 time needed to be spent to complete single task part\n");
    fprintf(stderr, "Where list contains at least one element from range 0 <= element <= 100, where element determines\n");
    fprintf(stderr, "students probability to need exceeded time on the task part. Also amount of this probabilities\n");
    fprintf(stderr, "determines amount of students.\n");
    exit(EXIT_FAILURE);
}

void studentHandler(int sigNo){
    isApproved = TRUE;
}

void parentHandler(int sigNo, siginfo_t* studInfo, void* _){
    studentsPID = studInfo->si_pid;
}

void sigchldHandler(int sigNo){
//    isChildDead = TRUE;
}

void setHandler(void (*f)(int), int SigNum){
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = f;

    if (sigaction(SigNum, &action, NULL))
        ERR("sigaction");
}

void setAdvHandler(void (*f)(int, siginfo_t*, void*), int SigNum){
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = f;

    if (sigaction(SigNum, &action, NULL))
        ERR("sigaction");
}

int findId(pid_t* idMap, int studCount, pid_t pid)
{
    for(int i = 0; i < studCount; ++i){
        if (idMap[i] == pid) return i;
    }

    return -1;
}


int* parentJob(pid_t* idMap, int studCount)
{
    int* exitStats;
    sigset_t mask, oMask;

    exitStats = (int*)malloc(sizeof(int) * studCount);

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oMask);

    int sC = studCount;
    while(sC)
    {
        sigsuspend(&oMask);
        printf("Teacher has accepted solution of student [ %d ]\n", studentsPID);

        if (kill(studentsPID, SIGUSR2)) ERR("KILL");

        pid_t studPid;
        int stat;
        while(sC && (studPid = waitpid(0, &stat, WNOHANG))){
            if (studPid < 0) ERR("waitpid"); 

            int index = findId(idMap, studCount, studPid);

            if (index == -1){
                fprintf(stderr, "Stud index not found on map\n");
                exit(EXIT_FAILURE);
            }

            exitStats[index] = WEXITSTATUS(stat);
            --sC;
        }
    }

    printf("[ TEACHER ] All students successfully ended their tasks\n");
    return exitStats;
}

int childJob(int id, long studentProb, int taskParts, int taskDuration)
{
    static const int msecInNsec = 1000000;
    static const int taskDurMult = 100 * msecInNsec;
    static const int issueDur = 50 * msecInNsec;
    static const int handRaiseInterval = 10 * msecInNsec;
    int issueCount = 0;

    srand(getpid());
    setHandler(studentHandler, SIGUSR2);
    printf("Student [%d, %d] has started doing task!\n", id, getpid());

    for(int i = 0; i < taskParts; ++i){
        printf("Student [%d, %d], is starting doing part %d of %d parts\n", id, getpid(), i + 1, taskParts);

        int tD = taskDuration;
        while(tD --> 0){ 
            struct timespec t = { .tv_sec = 0, .tv_nsec = taskDurMult };
            while(nanosleep(&t, &t));

            int ticket = 1 + rand() % 100;
            if (ticket < studentProb){ // student has bad luck :(
                printf("Student [%d, %d] occurred issue during task (%d)!\n", id, getpid(), ++issueCount);

                struct timespec iT =  { .tv_sec = 0, .tv_nsec = issueDur };
                while(nanosleep(&iT, &iT));
            }
        }

        printf("Student [%d, %d] has finished part %d of %d parts\n", id, getpid(), i + 1, taskParts);

        while(TRUE){
            if (kill(getppid(), SIGUSR1)) ERR("kill");

            struct timespec hRInt = { .tv_sec = 0, .tv_nsec = handRaiseInterval };
            nanosleep(&hRInt, NULL);

            if (isApproved == TRUE) {
                isApproved = FALSE;
                break;
            }
        }
    }

    printf("Student [%d, %d] has finished all tasks!\n", id, getpid());
    return issueCount;
}

void printResult(int* stats, int studCount, pid_t* map)
{
    int totalIssues = 0;

    printf("No.   | Student ID | Issue count\n");
    for (int i = 0 ; i < studCount; ++i){
        printf("  %3d | %10d | %d\n", i+1, map[i], stats[i]);
        totalIssues += stats[i];
    }
    printf("Total issues: %d\n", totalIssues);
}

pid_t* spawnChilds(int studentCount, long* studentsProb, int taskParts, int taskDuration)
    // IMPORTANT: returns allocated data, expected to be freed
{
    pid_t* idMap = malloc(sizeof(pid_t) * studentCount);
    if (idMap == NULL) ERR("malloc");

    for (int i = 0; i < studentCount; ++i){
        long prob = studentsProb[i];
        int status = fork();

        if (status < 0) ERR("fork");
        if (status == 0){ // child process
            free(idMap);
            int issueCount = childJob(i, prob, taskParts, taskDuration);
            exit(issueCount);
        }

        idMap[i] = status;
    }

    return idMap;
}

int main(int argc, char** argv) {
    int taskParts, taskDuration, studentCount;
    long* studentsProb;
    if (argc < 4) usage(argv);

    taskParts = atoi(argv[1]); taskDuration = atoi(argv[2]);
    studentCount = argc - 3;

    if (taskParts < 1 || taskParts > 10 || taskDuration < 1 || taskDuration > 10) usage(argv);
    studentsProb = (long*) malloc(sizeof(long) * studentCount);
    if (studentsProb == NULL) ERR("malloc");

    for (size_t i = 3; i < argc; ++i){
        errno = 0;
        long num = strtol(argv[i], NULL, 10);
        if (errno != 0 || num < 0 || num > 100) {
            free(studentsProb);
            usage(argv);
        }
        studentsProb[i - 3] = num;
    }

    pid_t * idMap = spawnChilds(studentCount, studentsProb, taskParts, taskDuration);
    setAdvHandler(parentHandler, SIGUSR1);
    setHandler(sigchldHandler, SIGCHLD);
    setvbuf(stdout, NULL, _IONBF, 0);
    int * eStat = parentJob(idMap, studentCount);
    printResult(eStat, studentCount, idMap);

    free(eStat);
    free(idMap);
    free(studentsProb);
    return EXIT_SUCCESS;
}