#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>

extern long long initialized[16];
extern long long un_initialized[2000];
extern long long* un_init_ptr1;
extern long long* un_init_ptr2;
extern int init_const;
extern void foo();
void step1();
void step2();
void step3();
void step4();
void step5();
void step6();
char* region;

int main(){
    char input = 'a';

    step1();
    while (input != 'n'){
        printf("Press n to go to Step 2\n");
        scanf(" %c", &input);
    }
    input = 'a';

    step2();
    while (input != 'n'){
        printf("Press n to go to Step 3\n");
        scanf(" %c", &input);
    }
    input = 'a';

    step3();
    while (input != 'n'){
        printf("Press n to go to Step 4\n");
        scanf(" %c", &input);
    }
    input = 'a';

    step4();
    while (input != 'n'){
        printf("Press n to go to Step 5\n");
        scanf(" %c", &input);
    }
    input = 'a';

    step5();
    while (input != 'n'){
        printf("Press n to go to Step 6\n");
        scanf(" %c", &input);
    }
    input = 'a';

    step6();
    while (input != 'n'){
        printf("Press n to end program\n");
        scanf(" %c", &input);
    }
    return 0;
}

void step1(){
    printf ("initialized address: 0x%lx \n", (unsigned long int) initialized);
    printf ("un_initialized address: 0x%lx \n", (unsigned long int) un_initialized);
    printf ("un_init_ptr1 address: 0x%lx \n", (unsigned long int) &un_init_ptr1);
    printf ("un_init_ptr2 address: 0x%lx \n", (unsigned long int) &un_init_ptr2);
    printf ("const init_const 0x%lx \n", (unsigned long int) &init_const);
    printf ("main_func 0x%lx \n", (unsigned long int) &main);
    printf ("step1_func 0x%lx \n", (unsigned long int) &step1);
    printf ("step2_func 0x%lx \n", (unsigned long int) &step2);
    printf ("step3_func 0x%lx \n", (unsigned long int) &step3);
    printf ("step4_func 0x%lx \n", (unsigned long int) &step4);
    printf ("step5_func 0x%lx \n", (unsigned long int) &step5);
    printf ("step6_func 0x%lx \n", (unsigned long int) &step6);
    printf ("foo_func 0x%lx \n", (unsigned long int) &foo);
    printf ("printf_func 0x%lx \n", (unsigned long int) &printf);
}

void step2(){
    un_init_ptr1 = malloc(2 * 1000000);
    un_init_ptr2 = malloc(2 * 1000000);
    printf ("address at un_init_ptr1 : 0x%lx \n", (unsigned long int) un_init_ptr1);
    printf ("address at un_init_ptr2 : 0x%lx \n", (unsigned long int) un_init_ptr2);
}

void step3(){
    foo(12500);
}

void step4(){
    FILE *fp = fopen("shm.txt", "w");
    fseek(fp, 2000000 , SEEK_SET);
    fputc('\0', fp);
    fclose(fp);
    int fd = open("shm.txt", O_RDWR);
    region = mmap(NULL, 2000000, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    printf ("address mapped mem : 0x%lx \n", (unsigned long int) region);
}

void step5(){
    for (int i = 0; i < 6000; i++){
        region[i] = 'l';
    }
}

void step6(){
    int* pg_start = (int*)0x400000;
    for (int i = 0; i < 1020; i = i + 4){
        printf("0x%lx: %08x ", (unsigned long int)(pg_start+i),htonl(pg_start[i]));
        printf("0x%lx: %08x ", (unsigned long int)(pg_start+i+1),htonl(pg_start[i+1]));
        printf("0x%lx: %08x ", (unsigned long int)(pg_start+i+2),htonl(pg_start[i+2]));
        printf("0x%lx: %08x\n", (unsigned long int)(pg_start+i+3),htonl(pg_start[i+3]));
    }
}