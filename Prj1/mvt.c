#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

struct mapArg{
  char* vectorFile;
  unsigned long* intermediate;
  FILE* divided_file;
};
typedef struct mapArg mapArg;

struct produceArg{
  int K;
  int vectorLen;
  unsigned long** intermediates;
  FILE* output_file;
};
typedef struct produceArg produceArg;

//delete unnecessary files after using them
void deleteFiles(int K){
  for (int i = 0; i < K; i++){
    char* buffer = (char*) malloc(sizeof(char) * 32);
    snprintf(buffer, sizeof(char) * 32, "split%i.txt", i);
    remove(buffer);
  }
}
//add txt to the end of the entered file names if they are not added in the params
char* addTxt(char* name){
  char* txt = ".txt";
  char* check = strstr(name, txt);
  int length = sizeof(name)/sizeof(name[0]);

  if (check == NULL){
    length = length + 4;
    char *buffer = (char*) malloc(length);
    strcpy(buffer, name);
    strcat(buffer, txt);
    return buffer;
  }
  return name;
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
//make the divided files to apply the mapper to
FILE** getDividedFiles(int s, int K, FILE* matrix_file){
  FILE** dividedFiles = (FILE**)malloc(K * sizeof(FILE*));
  int i, j;
  unsigned long val;
  for (int o = 0; o < K; o++){
    if( access(createSplitFileName(o), F_OK ) != -1 ) {
      dividedFiles[o] = fopen(createSplitFileName(o), "r+");
    } else {
      FILE* temp = fopen(createSplitFileName(o), "w");
      fclose(temp);
      dividedFiles[o] = fopen(createSplitFileName(o), "r+");
    }
    
    if (o == K - 1){
      while (fscanf(matrix_file, "%d %d %lu\n", &i, &j, &val) != EOF)
        fprintf(dividedFiles[o], "%d %d %lu\n", i, j, val);
    }
    else{
      for (int p = 0; p < s; p++){
        if (fscanf(matrix_file, "%d %d %lu\n", &i, &j, &val) != EOF)
          fprintf(dividedFiles[o], "%d %d %lu\n", i, j, val);
        else
          break;
      }
    }
    fseek(dividedFiles[o], 0, SEEK_SET);
  }
  return dividedFiles;
}
//get the vector array from the vector file
unsigned long* getVector(FILE* vector_file, int vectorLen){
  unsigned long* vector = (unsigned long*) malloc(sizeof(unsigned long) * vectorLen);
  int index;
  unsigned long val;
  for (int i = 0; i < vectorLen; i++){
    fscanf(vector_file, "%d %lu\n", &index, &val);
    vector[index - 1] = val;
  }
  fseek(vector_file, 0, SEEK_SET);
  return vector;
}
//map a divided matrix file to its part result
// fflush (stdout); 
void* mapper (void* myArgs)
{
  mapArg* mapArgs = (mapArg*)myArgs;
  FILE* vector_file = fopen(mapArgs->vectorFile,"r");
  int vectorLen = countLines(vector_file);
  unsigned long* vector = getVector(vector_file, vectorLen);
  fclose(vector_file);

  int dividedFileLen = countLines(mapArgs->divided_file);
  int i, j;
  unsigned long val;
  for (int o = 0; o < dividedFileLen; o++){
    fscanf(mapArgs->divided_file, "%d %d %lu\n", &i, &j, &val);
    mapArgs->intermediate[i - 1] = mapArgs->intermediate[i - 1] + (val * vector[j - 1]);
  }
  pthread_exit(NULL);
  return 0;
}
//reduce the part results into one file
void* reducer (void* myArgs)
{
  produceArg* produceArgs = (produceArg*)myArgs;
  unsigned long result[produceArgs->vectorLen];
  for (int i = 0; i < produceArgs->vectorLen; i++){
    result[i] = 0;
  }

  for (int i = 0; i < produceArgs->K; i++){
    for (int j = 0; j < produceArgs->vectorLen; j++){
      result[j] = result[j] + produceArgs->intermediates[i][j];
    }
  }
  for (int i = 0; i < produceArgs->vectorLen; i++){
    fprintf(produceArgs->output_file, "%d %lu\n", (i + 1), result[i]);
  }
  return 0;
}

int main(int argc, char** argv)
{
  //ensure that the arguments are given correctly
  if (argc != 5){
    printf ("number of arguments given are not accurate. Please try again.\n");
    exit(0);
  }
  //time to do the whole task
  struct timeval start, end;
  long diff;
  gettimeofday(&start, NULL);
  //get the names of the files and the value of K
  // char* matrixFile = argv[1];
  // char* vectorFile = argv[2];
  // char* outputFile = argv[3];
  int K = atoi(*(argv + 4));
  //open the input and output files
  FILE* matrix_file = fopen(argv[1],"r");
  FILE* output_file = fopen(argv[3],"w");
  //get the size of the array
  FILE* vector_file = fopen(argv[2],"r");
  int vectorLen = countLines(vector_file);
  fclose(vector_file);
  //get the vaues of L and s respectively
  int L = countLines(matrix_file);
  int s = (int)(L/(K));
  //make an array of files which will be mapped by sub processes
  FILE** dividedFiles = getDividedFiles(s, K, matrix_file);
  //K threads for mapping
  unsigned long** intermediates = (unsigned long**)malloc(K * sizeof(unsigned long*));
  for (int i = 0; i < K; i++){
    intermediates[i] = calloc(vectorLen, sizeof(unsigned long));
  }
  pthread_t tid[K];
  for (int w = 0; w < K; w++) {
    mapArg* myArgs = (mapArg*)malloc(sizeof(mapArg));
    myArgs->divided_file = dividedFiles[w];
    myArgs->intermediate = intermediates[w];
    myArgs->vectorFile = argv[2];
    pthread_create(&tid[w], NULL, (void*) mapper, (void*) myArgs);
  }
  // wait for the threads to stop executing
  for (int w = 0; w < K; w++){
    pthread_join(tid[w], NULL);
  } 
	
  //create the reducer thread
  pthread_t reduce;
  produceArg* myProdArg = (produceArg*)malloc(sizeof(produceArg));
  myProdArg->intermediates = intermediates;
  myProdArg->K = K;
  myProdArg->output_file = output_file;
  myProdArg->vectorLen = vectorLen;
  pthread_create(&reduce, NULL, (void*) reducer, (void*) myProdArg);
  pthread_join(reduce, NULL);
  //close close all files
  for (int i = 0; i < K; i++){
    fclose(dividedFiles[i]);
  }
  fclose(matrix_file);
  fclose(output_file);
  // free(matrixFile);
  // free(vectorFile);
  // free(outputFile);
  for (int i = 0; i < K; i++){
    free(intermediates[i]);
  }
  free(intermediates);
  deleteFiles(K);
  //getting ending time
  gettimeofday(&end, NULL);
  diff = ((end.tv_sec * 1000000) + end.tv_usec) - ((start.tv_sec * 1000000) + start.tv_usec);
  printf("%ld microseconds\n", diff);
  exit(0);
}




