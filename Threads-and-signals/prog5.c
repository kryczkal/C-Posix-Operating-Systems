/*
 * This program writes a specified number of blocks of random data, each of a specified size, to a file.
 * The program takes four command-line arguments: a delay in nanoseconds for child processes to sleep, the number of blocks to write, the size of each block in megabytes, and the output file name.
 *
 * Key functionalities include:
 * - Setting up a signal handler for SIGUSR1 to increment a counter each time the signal is received.
 * - Creating a child process that periodically sends SIGUSR1 to the parent process.
 * - The parent process performs bulk reads from /dev/urandom and bulk writes to the specified file.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define ERR(str) (fprintf(stderr, "Error occurred on line: %d in file: %s\n", __LINE__, __FILE__), perror(str) \
        ,kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t u1Count = 0;

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void u1Handler(int sigNum){
    ++u1Count;
}

void setHandler(void (*f)(int), int sigNum)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;

    if (sigaction(sigNum, &act, NULL))
        ERR("Sigaction");
}

void childJob(int n)
{
    struct timespec t = { .tv_sec = 0, .tv_nsec = n * 1000 };

    while(1)
    {
        struct timespec tt = t;
        while(nanosleep(&tt, &tt));

        if (kill(getppid(), SIGUSR1)){
            ERR("kill");
        }

        fprintf(stderr, "[ CHILD: %d ] Send SIGUSR1 signal to parent...\nTotal count: %d\n", getpid(), ++u1Count);
    }
}

void parentJob(int blocks, int blockSize, char* fileName)
{
    blockSize *= 1024 * 1024; // MB
    int oFd;
    int iFd;
    ssize_t totalWrote = 0;
    char* buff;

    if ((oFd = open(fileName, O_WRONLY | O_CREAT, 0777)) < 0){
        ERR("open on file");
    }

    if ((iFd = open("/dev/urandom", O_RDONLY)) < 0){
        ERR("open on random");
    }

    if ((buff = malloc(blockSize)) == NULL){
        ERR("malloc");
    }

    while(blocks --> 0){
        ssize_t bytesRead = read(iFd, buff, blockSize);

        if (bytesRead == -1)
            ERR("read");

        ssize_t bytesWrote = write(oFd, buff, bytesRead);
        fprintf(stderr, "Bytes wrote this time: %zd\n", bytesWrote);

        if (bytesWrote == -1)
            ERR("write");

        totalWrote += bytesWrote;
        if (fprintf(stderr, "Total blocks written: %zd. Signals counters: %d\n", totalWrote, u1Count) < 0)
            ERR("fprintf");
    }

    if (close(iFd) || close(oFd))
        ERR("close");

    free(buff);
    if (kill(0, SIGTERM)) ERR("Kill");
}

void spawnChildren(int n){
    int status = fork();

    if (status < 0){
        ERR("fork");
    }
    if (status == 0){
        childJob(n);
        exit(EXIT_SUCCESS);
    }
}


int main(int argc, char** argv) {
    int n, blocks, blockSize;
    char *name;

    if (argc != 5) {
        fprintf(stderr, "[ ERROR ] Program expects 3 arguments only..\n");
        exit(EXIT_FAILURE);
    }

    n = atoi(argv[1]);
    blocks = atoi(argv[2]);
    blockSize = atoi(argv[3]);
    name = argv[4];

    if (n <= 0 || blocks <= 0 || blockSize <= 0) {
        fprintf(stderr, "Passed arguments cant be a negative value...\n");
        exit(EXIT_FAILURE);
    }

    setHandler(u1Handler, SIGUSR1);

    spawnChildren(n);
    parentJob(blocks, blockSize, name);

    return EXIT_SUCCESS;
}