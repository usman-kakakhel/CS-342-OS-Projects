#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define READ_END 0
#define WRITE_END 1

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
void mapping(char* vectorFile, FILE* divided_file, int* fd){
  FILE* vector_file = fopen(vectorFile,"r");
  int vectorLen = countLines(vector_file);
  unsigned long* result = (unsigned long*) malloc(sizeof(unsigned long) * vectorLen);
  for (int o = 0; o < vectorLen; o++)
    result[o] = 0;
  
  unsigned long* vector = getVector(vector_file, vectorLen);

  int dividedFileLen = countLines(divided_file);
  int i, j;
  unsigned long val;
  for (int o = 0; o < dividedFileLen; o++){
    fscanf(divided_file, "%d %d %lu\n", &i, &j, &val);
    result[i - 1] = result[i - 1] + (val * vector[j - 1]);
  }
  fclose(vector_file);
  close(fd[READ_END]);
  write(fd[WRITE_END], result, vectorLen * sizeof(unsigned long));
  close(fd[WRITE_END]);
}
// //reduce the part results into one file
void produceResult(FILE* output_file, int K, int vectorLen, unsigned long** readBuffer){
  unsigned long result[vectorLen];
  for (int i = 0; i < vectorLen; i++){
    result[i] = 0;
  }

  for (int i = 0; i < K; i++){
    for (int j = 0; j < vectorLen; j++){
      result[j] = result[j] + readBuffer[i][j];
    }
  }
  for (int i = 0; i < vectorLen; i++){
    fprintf(output_file, "%d %lu\n", (i + 1), result[i]);
  }
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
  int s = (int)(L/(K * 1.0));
  //make an array of files which will be mapped by sub processes
  FILE** dividedFiles = getDividedFiles(s, K, matrix_file);
  //make the pipes
  unsigned long** readBuffer = (unsigned long**)malloc(K * sizeof(unsigned long*));
  int** fd = (int**) malloc(K * sizeof(int*));
  for (int i = 0; i < K; i++){
    readBuffer[i] = (unsigned long*)malloc(vectorLen * sizeof(unsigned long));
    fd[i] = (int*)malloc(2 * sizeof(int));
    if (pipe(fd[i]) == -1){
      fprintf(stderr, "pipe Failed");
    }
  }
  //fork parent to get K children and map the divided files to their temp files
  pid_t n = 0; 
  for (int w = 0; w < K; w++) {
    n = fork(); 
    if (n == 0) {
      mapping(argv[2], dividedFiles[w], fd[w]);
      exit(0);
    }
    else{
      close(fd[w][WRITE_END]);
      read(fd[w][READ_END], readBuffer[w], vectorLen * sizeof(unsigned long));
      close(fd[w][READ_END]);
    }
  }
  // wait for the children to stop executing
  for (int w = 0; w < K; w++){
    wait(NULL);
  } 
	  
  //create the reducer child
  n = fork();
  if (n == 0){
    produceResult(output_file, K, vectorLen, readBuffer);
    exit(0);
  }
  else
    wait(NULL);
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
    free(readBuffer[i]);
    free(fd[i]);
  }
  free(readBuffer);
  free(fd);
  deleteFiles(K);
  //getting ending time
  gettimeofday(&end, NULL);
  diff = ((end.tv_sec * 1000000) + end.tv_usec) - ((start.tv_sec * 1000000) + start.tv_usec);
  printf("%ld microseconds\n", diff);
  exit(0);
}

