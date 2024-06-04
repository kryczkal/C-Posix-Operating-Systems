/*
 * This program demonstrates the use of command-line options processing in C using the getopt function.
 * It supports two options: '-t' to specify a multiplier and '-n' to specify a string to be printed.
 * 
 * The '-t' option allows the user to set a multiplier value (default is 1) that affects how many times the string provided with '-n' will be printed.
 * The '-n' option specifies the string to be printed, which will be printed 'mult' times as determined by the '-t' option.
 * 
 * If the '-t' option is provided, its argument is converted to an integer and used to set the multiplier.
 * If the '-n' option is provided, the program prints the specified string, repeated according to the multiplier value.
 * 
 * The program expects that each option will have a corresponding argument and checks for the presence of additional non-option arguments after all options are processed.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char* argv[]){
    int mult = 1;
    int opt;

    while ((opt = getopt(argc, argv, "t:n:")) != -1){
        switch (opt) {
            case 't':
                mult = atoi(optarg);
                break;
            case 'n':
                for(int i = 0; i < mult; ++i)
                    printf("%s\n", optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (argc > optind)
        exit(EXIT_FAILURE);
    return EXIT_SUCCESS;
}