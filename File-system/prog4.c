/*
 * This program copies the content of one file to another.
 * 
 * The program takes two command-line arguments:
 *   - The source file (srcFile) from which the content will be copied.
 *   - The destination file (dstFile) to which the content will be copied.
 * 
 * The main steps involved in the program are as follows:
 *   1. It checks for the correct number of command-line arguments. If the number is incorrect, it displays usage information and exits.
 *   2. It opens the source file for reading and the destination file for writing. If either file cannot be opened, it reports an error and exits. The destination file is created if it does not exist, and if it does, its content is truncated.
 *   3. It copies the content from the source file to the destination file using a buffer. This is done in a loop that reads a chunk of data into the buffer from the source file and then writes that buffer to the destination file, until the end of the source file is reached.
 *   4. It closes both the source and destination files. If an error occurs during closing, it reports an error and exits.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ERR(errStr) (perror(errStr), fprintf(stderr, "File: %s, on line: %d", __FILE__, __LINE__), exit(EXIT_FAILURE))

void Usage(const char * name){
    printf("Usage: %s srcFile dstFile \nwhere:\nsrcFile - file FROM which content will be copied\ndstFile - file TO which content will be copied\n");
    exit(EXIT_FAILURE);
}

void copyContent(int fdFrom, int fdTo){
    char buff[4096];
    ssize_t cnt;

    cnt = TEMP_FAILURE_RETRY(read(fdFrom, buff, 4096));

    while( cnt > 0){
        TEMP_FAILURE_RETRY(write(fdTo, buff, cnt));
        cnt = TEMP_FAILURE_RETRY(read(fdFrom, buff, 4096));
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
        Usage(argv[0]);


    int fdIn = open(argv[1], O_RDONLY);
    int fdOut = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fdIn < 0 || fdOut < 0 )
        ERR("open");

    copyContent(fdIn, fdOut);

    if (close(fdIn) < 0 || close(fdOut) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}