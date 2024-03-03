/*
 * This C program is a directory analysis tool designed to operate on Unix-like systems. It utilizes the nftw (file tree walk) library to traverse directories and perform analysis on file nodes. 
 *
 * - Parsing command-line arguments to accept a list of directories to analyze (`-d DIR`), an optional output file name (`-o OUT`), and a flag to enable recursive traversal (`-r`).
 * - Validating input file or directory names to ensure they contain only lowercase letters or dots.
 * - Counting normal files and symbolic links (symlinks) in the specified directories. It also identifies the file with the longest name during the traversal.
 * - Supporting both non-recursive and recursive directory traversal based on the user's choice.
 * - Outputting the analysis results, which include the total number of normal files, the total number of symlinks, and the longest filename found, to either a specified file or stdout.
 */

#define _XOPEN_SOURCE 500

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <errno.h>

#define FALSE 0
#define TRUE 1

#define ERR(errStr) (perror(errStr), fprintf(stderr, "In file: %s, on line: %d", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char* progName)
{
    fprintf(stderr, "Usage: %s [ -d DIR ] [ -o OUT ] [ -r ]\nWhere:\n- DIR - desired directory to be checked. Any number of occurrences\n- OUT - name of file to which program output will be written. Zero or one occurrence\n- r - zero or one occurrence - enables recursive directory search\n", progName);
    exit(EXIT_FAILURE);
}

#define IS_NOT_LOWER_CASE(x) (x < 'a' || x > 'z')
#define IS_NOT_DOT(x) (x != '.')


char* checkName(char* name)
    // checks for correct name input as argument that means name is constructed only with small case alphabet or dots
    // RETURN: NULL on failure and name on success
{
    size_t len = strlen(name);

    for (size_t i = 0; i < len; ++i){
        if (IS_NOT_DOT(name[i]) && IS_NOT_LOWER_CASE(name[i]))
            return NULL;
    }

    return name;
}

char* getFName(const char* name)
{
    char* fName = strrchr(name, '/');

    if (fName == NULL) fName = name;
    else fName++;

    size_t len = strlen(fName);
    char* outName = (char*)malloc(len + 1);
    strcpy(outName, fName);

    return outName;
}

struct nodeCountData{
    int NormalFiles;
    int SymlinkFiles;
    size_t LongestLen;
    char* LongestName;
    int isRecu;
}nodeCountData;

int nodeCount(const char* fPath, const struct stat* sb, int typeFlag, struct FTW* ftwbuf)
{
    if (!nodeCountData.isRecu && ftwbuf->level > 1)
        return 0;

    if (typeFlag == FTW_F)
        nodeCountData.NormalFiles++;
    else if (typeFlag == FTW_SL)
        nodeCountData.SymlinkFiles++;

    if (typeFlag != FTW_D)
    {
        char *name = getFName(fPath);
        size_t len = strlen(name);

        if (len > nodeCountData.LongestLen) {
            free(nodeCountData.LongestName);

            nodeCountData.LongestLen = len;
            nodeCountData.LongestName = name;
        } else free(name);
    }

    return 0;
}

void processPath(char* path, int isRecu, FILE* output)
{
    nodeCountData.NormalFiles = nodeCountData.SymlinkFiles = nodeCountData.LongestLen = 0;
    nodeCountData.LongestName = NULL;
    nodeCountData.isRecu = isRecu;

    if (nftw(path, nodeCount, 64, FTW_PHYS) < 0) {
        perror("nftw");
        fprintf(stderr, "Not possible to fully process path: %s\n", path);
        return;
    }

    fprintf(output, "Path: %s\n Normal files: %d\n Symlinks: %d\n Longest name: %s\n", path, nodeCountData.NormalFiles, nodeCountData.SymlinkFiles, nodeCountData.LongestName);
}

int main(int argc, char** argv) {
    int isRecursive = FALSE;
    char* outName = NULL;
    char** paths = (char**)malloc(sizeof(char*) * (argc - 1));
    int pathsPos = 0;
    int c;

    // option processing
    while((c = getopt(argc, argv, "d:o:r")) != -1){
        switch (c) {
            case 'd':
                paths[pathsPos++] = optarg;
                break;
            case 'o':
                if (outName != NULL)
                    usage(argv[0]);

                outName = checkName(optarg);

                // checkName returns NULL on failure
                if (!outName)
                    usage(argv[0]);
                break;
            case 'r':
                if (isRecursive != FALSE)
                    usage(argv[0]);

                isRecursive = TRUE;
                break;
            default:
                usage(argv[0]);
        }
    }

    if (optind != argc)
        usage(argv[0]);

    // Setting up desired output
    FILE* output = stdout;

    if (outName != NULL){
        if (!(output = fopen(outName, "w"))){
            perror("fopen failure on output file");
            fprintf(stderr, "Continuing with stdout\n");
            output = stdout;
        }
    }

    // processing list of paths
    for(int i = 0; i < pathsPos; ++i)
        processPath(paths[i], isRecursive, output);


    // cleaning
    fclose(output);
    free(paths);

    return EXIT_SUCCESS;
}