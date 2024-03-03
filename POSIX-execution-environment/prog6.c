/*
 * This program demonstrates how to access and print the environment variables of the process in C.
 * It uses the 'environ' external global variable, which is a NULL-terminated array of strings.
 * Each string is a key-value pair in the format 'KEY=value', representing the environment variables available to the process.
 * 
 * The program iterates through the 'environ' array and prints each environment variable to standard output.
 * The iteration continues until a NULL pointer is encountered, indicating the end of the array.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char** environ;

int main(){
     int i = 0;
     char* s;

    while(environ[i]){
        printf("%s\n", environ[i++]);
    }

    return EXIT_SUCCESS;
}