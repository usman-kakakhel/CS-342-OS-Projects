#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char** argv){
    //ensure that the arguments are given correctly
    if (argc != 2){
        printf ("number of arguments given are not accurate. Please try again.\n");
        exit(0);
    }
    int N = atoi(argv[1]);
    
    int fd = open("vDisk", O_CREAT|O_WRONLY|O_TRUNC);
    if (fd < 0){
        printf ("File could not be created.\n");
        exit(0);
    }

    for (int i = 0; i < N; i++){
        for (int j = 0; j < 512; j++){
            if (write(fd, "dilwhich", 8) != 8){
                printf("Failed to write into file.\n");
            }
        }
    }

    close(fd);
    return 0;
}