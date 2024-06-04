/*
 * The program first saves the current working directory, then changes to each directory provided as an argument (if any) using chdir, scans it, and finally returns to the original directory.
 * 
 * Scanning involves opening the directory with opendir, reading each entry with readdir, and using lstat to obtain information about each entry. Based on this information, the program counts files, directories, symbolic links, and other types of entries.
 * 
 * It uses two scanning implementations: 
 *   - _scanDirPosix(), which is a POSIX-compliant method that uses lstat to determine the type of each directory entry. 
 *   - _scanDirArch(), an architecture-specific method intended to demonstrate how directory entries might be directly classified using dirent's d_type field (not used in the final scanDir function due to its reliance on features that may not be supported on all filesystems).
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>

#define ERR(errStr) ( perror(errStr), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void _scanDirPosix(){
    DIR* dirp;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");
    errno = 0;

    struct dirent *dirEntry;
    struct stat entryStat;
    int dirs;
    int files;
    int links;
    int others;

    dirs = files = links = others = 0;
    while ((dirEntry = readdir(dirp)) != NULL){
        if (lstat(dirEntry->d_name, &entryStat))
            ERR("lstat");

        if (S_ISDIR(entryStat.st_mode)){
            ++dirs;
        }
        else if (S_ISLNK(entryStat.st_mode)){
            ++links;
        }
        else if (S_ISREG(entryStat.st_mode)){
            ++files;
        }
        else{
            ++others;
        }
        errno = 0;
    }

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

    printf("Posix implementation\nFiles: %d; Dirs: %d; Links: %d; Other %d;\n", files, dirs, links, others);
}

void _scanDirArch(){
    DIR* dirp;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");
    errno = 0;

    struct dirent *dirEntry;
    int dirs;
    int files;
    int links;
    int others;

    dirs = files = links = others = 0;
    while ((dirEntry = readdir(dirp)) != NULL){
        if (S_ISDIR(dirEntry->d_type)){
            ++dirs;
        }
        else if (S_ISLNK(dirEntry->d_type)){
            ++links;
        }
        else if (S_ISREG(dirEntry->d_type)){
            ++files;
        }
        else if (dirEntry->d_type & DT_UNKNOWN == DT_UNKNOWN){
            ERR("dirent feature not supported");
        }
        else{
            ++others;
        }
        errno = 0;
    }

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

    printf("Arch exclusive implementation\nFiles: %d; Dirs: %d; Links: %d; Other %d;\n", files, dirs, links, others);
}

void scanDir(){
    _scanDirPosix();
}

int main(int argc, char** argv)
{
    char path[PATH_MAX];

    if (getcwd(path, PATH_MAX) == NULL)
        ERR("getcwd");

    for (int i = 1; i < argc; ++i)
    {
        if (chdir(argv[i]))
            ERR("chdir");

        printf("%s\n", argv[i]);
        scanDir();

        if (chdir(path))
            ERR("chdir");
    }
}
