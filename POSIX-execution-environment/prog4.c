/*
 * This program demonstrates conditional compilation and basic input validation in C.
 * It expects exactly two command-line arguments (excluding the program name) and repeats the first argument a specified number of times.
 * The number of repetitions is determined by the second argument, which should be a positive integer.
 * 
 * The program uses conditional compilation directives to switch between using `long` and `int` data types for the count variable,
 * depending on whether the ULONG macro is defined. If ULONG is defined, it uses `strtol` for numeric conversion and checks for errors during the conversion.
 * Otherwise, it uses `atoi` for conversion without error checking.
 * 
 * It validates the number of command-line arguments, ensuring exactly two arguments are passed, and verifies that the numeric argument represents a positive value.
 * If any validation fails, it prints an error message to `stderr` and exits with `EXIT_FAILURE`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define EXPECTED_ARGUMENTS 3
//#define ULONG

int main(int argc, const char* argv[]){

#ifdef ULONG
    typedef long count_t;
#else
    typedef int count_t;
#endif
    count_t count;

    if (argc != EXPECTED_ARGUMENTS){
        fprintf(stderr, "[ERROR] Program expects 2 arguments to be passed!!\n");
        exit(EXIT_FAILURE);
    }


#ifdef ULONG
    errno = 0;
    count = strtol(argv[2], NULL, 10);

    if (errno != 0){
        perror("Numeric conversion failed");
        exit(EXIT_FAILURE);
    }
#else
    count = atoi(argv[2]);
#endif

    if (count < 1){
        fprintf(stderr, "[ERROR] Count should be above 0\n");
        exit(EXIT_FAILURE);
    }

    for (count_t i = 0; i < count; ++i){
        printf("%s\n", argv[1]);
    }

    return EXIT_SUCCESS;
}