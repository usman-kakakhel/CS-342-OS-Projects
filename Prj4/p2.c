#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>


int main(int argc, char** argv){
    //ensure that the arguments are given correctly
    if (argc != 2){
        printf ("number of arguments given are not accurate. Please try again.\n");
        exit(0);
    }
    char* address = argv[1];
    if (address[strlen(address) - 1] != '/'){
        strcat(address, "/");
    }
    DIR *pDir;
    struct dirent *pDirent;
    struct stat fileStat;
    pDir = opendir (address);
    if (!pDir){
        printf ("Wrong Directory address. Please try again.\n");
        exit(0);
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        int addressLen = strlen(address);
        char tempAdd[addressLen + 1];
        strcpy(tempAdd, address);
        strcat(tempAdd, pDirent->d_name);
        printf ("------------------------------\n");
        stat(tempAdd,&fileStat);
        printf("Name: \t\t\t%s\n", pDirent->d_name);
        printf("User ID: \t\t%d\n", fileStat.st_uid);
        printf("Inode Number: \t\t%ld\n", fileStat.st_ino);
        if (S_ISREG(fileStat.st_mode)) {
            printf("File Type: \t\tFile\n");
        }
        else if (S_ISDIR(fileStat.st_mode)) {
            printf("File Type: \t\tDirectory\n");
        }
        else if (S_ISCHR(fileStat.st_mode)) {
            printf("File Type: \t\tCharacter Device\n");
        }
        else if (S_ISBLK(fileStat.st_mode)) {
            printf("File Type: \t\tBlock Device\n");
        }
        else if (S_ISFIFO(fileStat.st_mode)) {
            printf("File Type: \t\tFIFO\n");
        }
        else if (S_ISLNK(fileStat.st_mode)) {
            printf("File Type: \t\tSymbolic Link\n");
        }
        else if (S_ISSOCK(fileStat.st_mode)) {
            printf("File Type: \t\tSocket\n");
        }
        printf("Number Of Blocks: \t%ld\n", fileStat.st_blocks);
        printf("Size in Bytes: \t\t%ld\n", fileStat.st_size);
    }

    closedir (pDir);
    return 0;
}