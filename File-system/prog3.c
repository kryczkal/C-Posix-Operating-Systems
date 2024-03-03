/*
 * This program demonstrates how to create a file with specific properties such as name, size, and permissions from command-line arguments.
 * It supports three options:
 *   -n NAME: Specifies the filename.
 *   -p OCTAL: Sets the file permissions using an octal value.
 *   -s SIZE: Defines the size of the file in bytes.
 * Additionally, it fills 10% of the file with random bytes, starting from the beginning, to simulate file content.
 *
 * The program uses getopt to parse command-line options and validates input for each option. It requires all three options to be provided for it to run successfully.
 * 
 * The MakeFile function creates the file with the specified name and size. It first sets the file's permissions mask using umask, then opens the file for writing. 
 * It writes '\0' bytes to achieve the desired file size, then seeks back to the beginning of the file to overwrite 10% of its size with random bytes.
 * 
 * Error handling is implemented throughout the program. It uses a custom macro ERR to print error messages and exit in case of failure. 
 * Before creating the file, it attempts to unlink any existing file with the same name to ensure that the new file can be created without issues.
 * 
 * Usage instructions are provided through the Usage function, which is called if the user provides incorrect arguments or requests help.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0

#define ERR(errStr) (perror(errStr), fprintf(stderr, "File: %s; line: %d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void Usage(char* filename)
{
    printf("Usage: %s [-n NAME] [-p OCTAL] [-s SIZE]\nWhere:\n-NAME - filename,\n-OCTAL - privileges of newly created file in octal format,\n-SIZE - size of file\n", filename);
    exit(EXIT_FAILURE);
}

void MakeFile(char* name, long size, long mask, unsigned short rand_perc)
{
    FILE* fp;
    mode_t oldPerm;

    oldPerm = umask(mask);

    if (!(fp = fopen(name, "w")))
        ERR("fopen");

    for (long i = 0; i < size; ++i)
        putc('\0', fp);

    long rand_chars = size * rand_perc / 100;

    if (fseek(fp, 0, SEEK_SET))
        ERR("fseek");

    for (long i = 0; i < rand_chars; ++i)
        putc(rand() % 255 - 128, fp);

    umask(oldPerm);

    if (fclose(fp))
        ERR("fclose");
}

int main(int argc, char** argv)
{
    int c;

    long mask = -1;
    char* name = NULL;
    long size = -1;

    while((c = getopt(argc, argv, "n:p:s:")) != -1)
    {
        switch (c) {
            case 'n':
                if (name != NULL)
                    Usage(argv[0]);

                name = optarg;
                break;
            case 'p':
                if (mask != -1)
                    Usage(argv[0]);

                errno = 0;
                mask = strtol(optarg, NULL, 8);

                if (errno != 0)
                    Usage(argv[0]);

                if (mask < 0)
                    Usage(argv[0]);

                break;
            case 's':
                if (size != -1)
                    Usage(argv[0]);

                errno = 0;
                size = strtol(optarg, NULL, 10);

                if (errno != 0)
                    Usage(argv[0]);

                if (size <= 0)
                    Usage(argv[0]);
                break;
            default:
                Usage(argv[0]);
        }
    }

    if (name == NULL || mask == -1 || size == -1)
        Usage(argv[0]);

    if (unlink(name) && errno != ENOENT)
        ERR("unlink");

    srand(time(NULL));
    MakeFile(name, size, mask, 10);

    return EXIT_SUCCESS;
}