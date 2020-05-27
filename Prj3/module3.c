#include <stdio.h>

void foo(int fin){
    if(fin > 0){
        long long temp = 1;
        if (fin == 12500 || fin == 1){
            printf ("address of var inside foo: 0x%lx \n", (unsigned long int) &fin);
        }
        foo(fin - 1);
    }
    return ;
}