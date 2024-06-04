/*
 * This program demonstrates the use of the nftw (new file tree walk) function to traverse directories and their subdirectories, counting and printing information about each entry encountered.
 * It operates on directories specified as command-line arguments. For each directory, it applies two main functions:
 *   - printNode: This function is called for each entry in the directory tree. It prints the entry's type (directory, file, symlink, or other) and name, with indentation corresponding to the entry's depth in the directory tree. The indentation is dynamically calculated to visually represent the directory structure.
 *   - countNode: Although defined, it's not used in the main logic but is intended to count the types of entries (directories, files, symbolic links, and others) encountered during the traversal.
 * 
 * The traversal is performed in a physical manner (FTW_PHYS flag), meaning that symbolic links to directories are not followed, to avoid potentially infinite loops in the presence of circular links.
 * 
 * Error handling is done through the ERR macro for critical errors, and a check is included after the nftw call to catch any permissions-related issues during the traversal.
 *
 * Note: The countNode function is set up for demonstration but not actively used in the program's main logic. Instead, the program focuses on visually representing the directory structure and the types of entries within it.
 */

#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#define MAXFD 20

#define ERR(errStr) (perror(errStr), fprintf(stderr, "In file: %s, on line: %d", __FILE__, __LINE__), exit(EXIT_FAILURE))

int dirs = 0, files = 0, links = 0, other = 0;

int countNode(const char*, const struct stat* sp, int type, struct FTW* ftwStruct){
    switch (type) {
        case FTW_DNR:
        case FTW_D:
            ++dirs;
            break;
        case FTW_F:
            ++files;
            break;
        case FTW_SLN:
        case FTW_SL:
            ++links;
            break;
        default:
            ++other;
    }

    return 0;
}

char* nTabs(int n){
    char* tab = (char*)malloc(n + 1);
    memset(tab, ' ', n);
    tab[n] = '\0';
    return tab;
}

char* getFName(const char* name)
{
    char* string = strrchr(name, '/');
    int nLen = strlen(string + 1);
    char* retStr = (char*)malloc(nLen + 1);

    strcpy(retStr, string + 1);

    return retStr;
}

int printNode(const char* name, const struct stat* sp, int type, struct FTW* ftwStruct){
    char* indent = nTabs(ftwStruct->level);
    char* fname = getFName(name);

    switch (type) {
        case FTW_DNR:
        case FTW_D:
            printf("%sDir %s:\n", indent, fname);
            break;
        case FTW_F:
            printf("%sFile %s\n", indent, fname);
            break;
        case FTW_SLN:
        case FTW_SL:
            printf("%sLink %s\n", indent, fname);
            ++links;
            break;
        default:
            printf("%sOther %s\n", indent, fname);
            ++other;
    }

    free(fname);
    free(indent);
    return 0;
}



int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i){
        if (nftw(argv[i], printNode, MAXFD, FTW_PHYS))
            fprintf(stderr, "Lack of permission\n");
        else
            printf("%s:\nfiles:%d\ndirs:%d\nlinks:%d\nother:%d\n", argv[i], files, dirs, links, other);

        dirs = files = links = other = 0;
    }
    return EXIT_SUCCESS;
}