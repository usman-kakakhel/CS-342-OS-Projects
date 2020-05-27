#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>


int main(int argc, char** argv){
    time_t t;
    srand((unsigned) time(&t));
    //ensure that the arguments are given correctly
    if (argc != 3){
        printf ("number of arguments given are not accurate. Please try again.\n");
        exit(0);
    }
    int K = atoi(argv[1]);
    int fd = open(argv[2], O_RDONLY);
    if (fd < 0){
        printf ("File could not be opened.\n");
        exit(0);
    }

    lseek(fd, 0, SEEK_END);
    struct stat fileStat;
    stat(argv[2],&fileStat);
    int size = fileStat.st_size;
    lseek(fd, 0, SEEK_SET);

    int range = size - K;
    if (range < 1){
        printf ("K greater than file Size, try again.\n");
    }

    struct timeval tv;
    suseconds_t totaltime = 0;
    for (int i = 0; i < 200; i++){
        gettimeofday(&tv, NULL);
        suseconds_t starttime = tv.tv_usec;
        lseek(fd, (int)(rand()%range), SEEK_SET);
        char data[K];
        read(fd, data, K);
        gettimeofday(&tv, NULL);
        totaltime = totaltime + (tv.tv_usec - starttime);
    }
    totaltime = totaltime/200;
    printf("Time taken: %ld micro Seconds\n", totaltime);

    close(fd);
    return 0;
}