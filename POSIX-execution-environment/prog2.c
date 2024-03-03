/*
 * This program demonstrates basic command-line argument processing in C.
 * It iterates through all the arguments passed to the program, including the program name itself,
 * and prints each argument to the standard output.
 * The purpose of this program is to showcase how command-line arguments can be accessed and utilized within a C program.
 * It returns EXIT_SUCCESS to indicate successful execution.
 */

#include <stdlib.h>
#include <stdio.h>

int main(int argc, const char* argv[]){
    for (int i = 0; i < argc; ++i){
        printf("%s\n", argv[i]);
    }
    return EXIT_SUCCESS;
}