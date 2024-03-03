/*
 * This program demonstrates file input operations in C, specifically reading lines from a file or standard input (stdin).
 * It defines a buffer size based on the maximum line length (MAX_LINE) expected, plus additional space for a null terminator and overflow checking.
 * 
 * The program can operate in two modes:
 * 1. If no command-line arguments are provided, it reads from standard input.
 * 2. If a single command-line argument is provided, it attempts to open and read from the specified file.
 * 
 * It validates the number of command-line arguments, allowing either none or exactly one argument representing a file name.
 * If more than one argument is provided, or if opening the specified file fails, it prints an error message and exits with `EXIT_FAILURE`.
 * 
 * For each line read, either from the file or stdin, it prints the line prefixed with "Hello! " to standard output.
 * This demonstrates basic text processing, including opening files, reading lines, and handling potential errors in file operations.
 */

#include <stdlib.h>
#include <stdio.h>

#define MAX_LINE 20
#define NULL_TERM_FIELD 1
#define OVERFLOW_CHECK_FIELD 1
#define BUFF_SIZE (MAX_LINE + NULL_TERM_FIELD + OVERFLOW_CHECK_FIELD)

int main(int argc, const char* argv[]){
    char buff[BUFF_SIZE];
    FILE* stream = stdin;

    if (argc == 2){
        stream = fopen(argv[1], "r");

        if (!stream){
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }
    }
    else if (argc > 2){
        fprintf(stderr, "Only one file should be passed. Given: %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    while(fgets(buff, BUFF_SIZE, stream)){
        printf("Hello! %s\n", buff);
    }

    return EXIT_SUCCESS;
}