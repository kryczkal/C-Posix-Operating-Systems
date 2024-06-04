/*
 * This program provides a simple command-line interface for interacting with the file system, offering operations like writing to, showing, and walking directories.
 * It supports the following commands:
 *   - A: Write to a file. It prompts for a file path and then allows writing to the file. The input is converted to uppercase before being saved.
 *   - B: Show the content of a file or list the contents of a directory. It handles files by displaying their content and directories by listing their contents.
 *   - C: Walk a directory recursively. It lists all files and directories within the specified directory, including subdirectories, using indentation to indicate depth.
 *   - D: Exit the program.
 */

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/dir.h>

#define FALSE 0
#define TRUE 1
#define INPUT_BUFF_SIZE 3

#define ERR(errStr) (perror(errStr), fprintf(stderr, "In file: %s, on line: %d", __FILE__, __LINE__), exit(EXIT_FAILURE))

void displayStartInfo()
{
    printf("Chose from one option below:\n- A. write\n- B. show\n- C. walk\n- D. exit\n");
}

char checkInput(char* input)
{
    if (input[1] != '\0') return '?';
    return toupper(input[0]);
}

void flushStdin(){
    int c;

    while((c = getchar()) != '\n' && c != EOF);
}

struct corr_path
{
    char* path;
    struct stat path_stat;
};

struct corr_path getPath()
{
    struct corr_path res = { .path = NULL };
    size_t size;
    size_t charSize;

    printf("Pass path to file\n");

    errno = 0;
    if ((charSize = getline(&res.path, &size, stdin)) == -1 && errno != 0)
        ERR("getline");
    res.path[charSize - 1] = '\0';

    if (lstat(res.path, &(res.path_stat))){
        perror("[ERROR] Path is not correct");
        res.path = NULL;
    }

    return res;
}

void writeCommand(struct corr_path path)
{
    if (!S_ISREG(path.path_stat.st_mode)){
        fprintf(stderr, "Passed file is not regular one\n");
        return;
    }

    FILE* file = fopen(path.path, "w");

    if (!file){
        perror("Failed to open passed file");
        return;
    }

    size_t lSize;
    char* line = NULL;
    size_t charsRead;

    printf("Writing to file...\n");

    errno = 0;
    charsRead = getline(&line, &lSize, stdin);

    while(charsRead != -1 && line[0] != '\n'){
        for(int i = 0; i < charsRead; ++i)
            line[i] = toupper(line[i]);

        if (fputs(line, file) < 0)
            ERR("fputs reported error, when writing to file");

        errno = 0;\
        charsRead = getline(&line, &lSize, stdin);
    }

    if (errno != 0)
        ERR("getline failed to read from stdin");

    free(line);
    fclose(file);
}

#define FILE_BUFF 4096

void readFile(char* path){
    int fd;

    if ((fd = open(path, O_RDONLY)) < 0){
        perror("Not able to open passed file");
        return;
    }

    char Buffer[FILE_BUFF];
    ssize_t charsRead;

    while((charsRead = read(fd, Buffer, FILE_BUFF)) > 0){
        // write to stdout
        if (write(1, Buffer, charsRead) < 0){
            ERR("cant write to file");
        }
    }

    if (close(fd))
        ERR("Not able to dealloc file descriptor");
}

void readDir(char* path){
    DIR* dirStream = opendir(path);

    if (dirStream == NULL){
        perror("Not able to open a directory");
        return;
    }

    errno = 0;
    struct dirent* dirEntry = readdir(dirStream);

    printf("Content of %s directory:\n", path);
    while(dirEntry != NULL){
        printf("%s\n", dirEntry->d_name);

        errno = 0;
        dirEntry = readdir(dirStream);
    }

    if (errno != 0)
        ERR("Not able to correctly read directory entry");

    if (closedir(dirStream))
        ERR("Not correctly closed directory stream");
}

void showCommand(struct corr_path path){
    if (S_ISREG(path.path_stat.st_mode))
        readFile(path.path);
    else if (S_ISDIR(path.path_stat.st_mode))
        readDir(path.path);
    else{
        fprintf(stderr, "File is not regular one neither directory\n");
    }
}

int printNode(const char* path, const struct stat* _, int type, struct FTW* recuInfo){
    for(int i = 0; i < recuInfo->level - 1; ++i)
        putchar(' ');

    if (type == FTW_D)
        putchar('+');
    else if (type == FTW_SL || type == FTW_F)
        putchar(' ');
    else {
        fprintf(stderr, "Directory not reachable: %s\n", strrchr(path, '/') + 1);
        return 0;
    }

    printf("%s\n", strrchr(path, '/') + 1);

    return 0;
}

void dirWalkCommand(struct corr_path path){
    if (!S_ISDIR(path.path_stat.st_mode)){
        fprintf(stderr, "passed file is not directory\n");
        return;
    }

    if (nftw(path.path, printNode, 64, FTW_PHYS) == -1){
        perror("Recursive dir walk occurred error on his way");
    }
}

int main(int argc, char** argv) {
    char inputBuff[INPUT_BUFF_SIZE];
    struct corr_path path;
    int shouldEnd = FALSE;

    displayStartInfo();
    while (scanf("%2s", inputBuff) != -1 && shouldEnd == FALSE) {
        flushStdin();
        char c = checkInput(inputBuff);
        switch (c) {
            case 'A':
                path = getPath();
                if (path.path) writeCommand(path);
                break;
            case 'B':
                path = getPath();
                if (path.path) showCommand(path);
                break;
            case 'C':
                path = getPath();
                if (path.path)dirWalkCommand(path);
                break;
            case 'D':
                shouldEnd = TRUE;
                break;
            default:
                fprintf(stderr, "[ERROR] Unrecognized option\n");
        }
        displayStartInfo();
    }

    return EXIT_SUCCESS;
}