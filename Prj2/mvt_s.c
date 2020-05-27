#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <semaphore.h>

//semaphores
sem_t** mutex;
sem_t** full;
sem_t** empty;

//structs
struct mat_val{
    int row;
    int col;
    long val;
};
typedef struct mat_val mat_val;

struct mapArg{
    int i;
    int B;
};
typedef struct mapArg mapArg;

struct redArg{
    int n;
    int K;
    int B;
    FILE* file;
};
typedef struct redArg redArg;

//global variables
long* vector;
mat_val** mat_buf;
int* in;
int* out;
int* fin;
int finCnt = 0;

//delete unnecessary files after using them
void deleteFiles(int K){
  for (int i = 0; i < K; i++){
    char* buffer = (char*) malloc(sizeof(char) * 32);
    snprintf(buffer, sizeof(char) * 32, "split%i.txt", i);
    remove(buffer);
    free(buffer);
  }
}

//count the number of lines in a given file
int countLines(FILE* file){
  int L = 0;
  while(!feof(file))
  {
    char nextChar = fgetc(file);
    if(nextChar == '\n'){
      L++;
    }
  }
  fseek(file, 0, SEEK_SET);
  return L;
}

//create a file name from its current position in the loop
char* createSplitFileName(int i){
  char* buffer = (char*) malloc(sizeof(char) * 32);
  snprintf(buffer, sizeof(char) * 32, "split%i.txt", i);
  return buffer;
}

//split fies creating
void createSplitFiles(int L, int K, FILE* file){
    //find value of s
    int s = (int)(L / K);
    int i, j;
    long val;
    for (int o = 0; o < K; o++){
        FILE* temp = fopen(createSplitFileName(o), "w");
        if (o == K - 1){
            while (fscanf(file, "%d %d %ld\n", &i, &j, &val) != EOF)
                fprintf(temp, "%d %d %ld\n", i, j, val);
        }
        else{
            for (int p = 0; p < s; p++){
                if (fscanf(file, "%d %d %ld\n", &i, &j, &val) != EOF)
                    fprintf(temp, "%d %d %ld\n", i, j, val);
                else
                    break;
            }
        }
        fclose(temp);
    }
}

long* readVector(int n, FILE* file){
    //initialize the global variable
    long* vec = (long*)calloc(n, sizeof(long));
    //read into global variable
    int i;
    long val;
    while (fscanf(file, "%d %ld\n", &i, &val) != EOF){
        vec[i-1] = val;
    }
    return vec;
}

void initializeSem(int B, int K){
    in = (int*)calloc(K , sizeof(int));
    out = (int*)calloc(K , sizeof(int));
    fin = (int*)calloc(K , sizeof(int));

    mutex = (sem_t**)malloc(K * sizeof(sem_t*));
    empty = (sem_t**)malloc(K * sizeof(sem_t*));
    full = (sem_t**)malloc(K * sizeof(sem_t*));
    
    for (int i = 0; i < K; i++){
        /* create and initialize the semaphores */
        mutex[i] = (sem_t*)malloc(sizeof(sem_t));
        full[i] = (sem_t*)malloc(sizeof(sem_t));
        empty[i] = (sem_t*)malloc(sizeof(sem_t));

        int check = sem_init(mutex[i], 0, 1);
        if (check < 0) {
            perror("can not create semaphore\n");
            exit (1); 
        }
        check = sem_init(full[i], 0, 0);
        if (check < 0) {
            perror("can not create semaphore\n");
            exit (1); 
        }
        check = sem_init(empty[i], 0, B);
        if (check < 0) {
            perror("can not create semaphore\n");
            exit (1); 
        }
    }
}

void* mapper(void* myArgs){
    mapArg* myMapArg = (mapArg*)myArgs;
    int myT = myMapArg->i;
    int B = myMapArg->B;
    FILE* split_file = fopen(createSplitFileName(myT),"r");
    
    int i, j;
    long val;

    while(fscanf(split_file, "%d %d %ld\n", &i, &j, &val) != EOF){
        sem_wait(empty[myT]);
        sem_wait(mutex[myT]);
        
        (mat_buf[myT] + in[myT])->row = i;
        (mat_buf[myT] + in[myT])->col = j;
        (mat_buf[myT] + in[myT])->val = val;
        in[myT] = (in[myT] + 1) % B;
        fin[myT]++;

        sem_post(mutex[myT]);
        sem_post(full[myT]);
    }
    sem_wait(mutex[myT]);
    finCnt++;
    sem_post(mutex[myT]);
    fclose(split_file);
    pthread_exit(NULL);
    return 0;
}

void* reducer(void* myArgs){
    redArg* myRedArg = (redArg*)myArgs;
    int K = myRedArg->K;
    int n = myRedArg->n;
    int B = myRedArg->B;
    FILE* output_file = myRedArg->file;
    long output[n];
    int ex;
    do{
        for (int i = 0; i < K; i++){
            sem_wait(mutex[i]);
            if (fin[i] > 0){
                sem_post(mutex[i]);
                sem_wait(full[i]);
                sem_wait(mutex[i]);
                output[((mat_buf[i] + out[i])->row) - 1] = output[((mat_buf[i] + out[i])->row) - 1] + (vector[((mat_buf[i] + out[i])->col) - 1] * (mat_buf[i] + out[i])->val);
                out[i] = (out[i] + 1) % B;
                fin[i]--;
                sem_post(mutex[i]);
                sem_post(empty[i]);
                sem_wait(mutex[i]);
            }
            sem_post(mutex[i]);
        }
        ex = 1;
        for (int i = 0; i < K; i++){
            sem_wait(mutex[i]);
            if (fin[i] != 0){
                ex = 0;
            }
            sem_post(mutex[i]);
        }
    }while(ex == 0 || finCnt < K);

    for (int i = 0; i < n; i++)
        fprintf(output_file, "%d %ld\n", (i+1), output[i]);
    
    pthread_exit(NULL);
    return 0;
}

int main(int argc, char** argv)
{
    //ensure that the arguments are given correctly
    if (argc != 6){
    printf ("number of arguments given are not accurate. Please try again.\n");
    exit(0);
    }
    
    //open the input, vector and output files
    FILE* matrix_file = fopen(argv[1],"r");
    FILE* vector_file = fopen(argv[2],"r");
    FILE* output_file = fopen(argv[3],"w");
    //get the K and B values
    int K = atoi(argv[4]);
    int B = atoi(argv[5]);
    deleteFiles(K); //remove any split files present
    //count the size of the vector - n
    int n = countLines(vector_file);
    //count the number of lines L of the matrix
    int L = countLines(matrix_file);
    //read vector file
    vector = readVector(n, vector_file);

    //create split files
    createSplitFiles(L, K, matrix_file);

    //initialize the buffers
    mat_buf = (mat_val**)malloc(K * sizeof(mat_val*));
    for (int i = 0; i < K; i++){
        mat_buf[i] = (mat_val*)calloc(B, sizeof(mat_val));
    }
    //create mapper and reducer threads
    initializeSem(B, K);
    pthread_t mapperT[K];
    mapArg** myMapArg = (mapArg**)malloc(K * sizeof(mapArg*));

    pthread_t reducerT;
    redArg* myRedArg = (redArg*)malloc(1 * sizeof(redArg));
    myRedArg->n = n;
    myRedArg->B = B;
    myRedArg->K = K;
    myRedArg->file = output_file;

    for (int i = 0; i < K; i++){
        myMapArg[i] = (mapArg*)malloc(1 * sizeof(mapArg));
        myMapArg[i]->i = i;
        myMapArg[i]->B = B;
        pthread_create(&mapperT[i], NULL, (void*) mapper, (void*) myMapArg[i]);
    }
    pthread_create(&reducerT, NULL, (void*) reducer, (void*) myRedArg);

    // wait for the threads to stop executing
    for (int w = 0; w < K; w++){
        pthread_join(mapperT[w], NULL);
    }
    pthread_join(reducerT, NULL);
    
    //close the semaphores
    for (int i = 0; i < K; i++){
        sem_destroy(mutex[i]);
        sem_destroy(empty[i]);
        sem_destroy(full[i]);
    }
    
    //free the reducer and mapper arguments
    free(myRedArg);
    for (int i = 0; i < K; i++){
        free(myMapArg[i]);
    }
    free(myMapArg);
    //free global variables
    free(vector);
    free(mat_buf);
    free(mutex);
    free(empty);
    free(full);
    free(in);
    free(out);
    free(fin);
    deleteFiles(K); //remove any split files present
    //close all files
    fclose(matrix_file);
    fclose(vector_file);
    fclose(output_file);
    return 0;
}