#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define TASKQUEUESIZE 500
#define MAX_ANGLE 360
#define JOBS_NUM 10
#define PI 3.14159265358979323846
#define CONSUMERSNUM 5
#define PRODUCERSNUM 3

typedef struct{
  void (*work)(void * args);
  void * args;
}workFunction;

typedef struct {
  workFunction buf[TASKQUEUESIZE];
  int len;
  long head, tail;
  int full, empty;
  pthread_cond_t notFull,notEmpty;
  pthread_mutex_t mut;
}TaskQueue;

typedef struct {
 int val;
 struct timeval start;
} function_args;

void *producer (void *args);
void *consumer (void *args);
void TaskQueueInit(TaskQueue *tq);
void AddTask(TaskQueue *tq, workFunction in);
void PopTask(TaskQueue *tq, workFunction *out);
void submitTask(TaskQueue *tq, workFunction wf);
void calculate_cosine(int * angle); 

TaskQueue tq;
double sum;
double average;
function_args arg_vector[TASKQUEUESIZE];
double delays [TASKQUEUESIZE];
int jobs_remaining = JOBS_NUM*PRODUCERSNUM;
int j=0;

int main(int argc, char *argv[]){
  srand(time(NULL)); 
  TaskQueueInit(&tq);
  int i;
  pthread_t pro [PRODUCERSNUM]; 
  pthread_t con [CONSUMERSNUM];
  
  for(i=0;i<PRODUCERSNUM;i++){
    if (pthread_create(&pro[i], NULL, &producer, NULL) != 0) {
      perror("Failed to create the thread");
      return 1;
  }
  }

  for(i=0;i<CONSUMERSNUM;i++){
    if (pthread_create(&con[i], NULL, &consumer, NULL) != 0) {
      perror("Failed to create the thread");
      return 2;
    }
  }
  
  for (i = 0; i < PRODUCERSNUM; i++) {
    if (pthread_join(pro[i], NULL) != 0) {
       return 3;     
    }
  }
    for (i = 0; i < CONSUMERSNUM; i++) {
      if (pthread_join(con[i], NULL) != 0) {
        return 4;
      }
   }
   //calculate the average
   for (i=0;i<JOBS_NUM*CONSUMERSNUM;i++){
    sum += delays[i]; 
   }
   average = (double)(((double)sum)/((double)JOBS_NUM));
   printf("\naverage delay=%.10f",average); 
}

void *producer(void *args){
  int i;
  workFunction wf;
  int angle;
  for (i = 0; i < JOBS_NUM ; i++) {
    angle = rand() % (MAX_ANGLE+ 1); // Generate a random angle between 0 and 360
    arg_vector[tq.tail].val = angle;
    wf.work = (void *)calculate_cosine;
    gettimeofday(&(arg_vector[tq.tail].start), NULL);
    wf.args = &arg_vector[tq.tail]; 
    pthread_mutex_lock(&tq.mut);
    while (tq.full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (&tq.notFull, &tq.mut);
    } 
    AddTask(&tq,wf);
    pthread_mutex_unlock (&tq.mut);
    pthread_cond_broadcast(&tq.notEmpty);
  }
  return (NULL);
}
void *consumer(void *args){
  int i=0;
  long seconds, microseconds;
  double elapsed_time;
  struct timeval end;
  workFunction wftemp;
  function_args temp_args;
  while (1) {
   
    pthread_mutex_lock (&tq.mut);
   
   if(jobs_remaining){
    if(tq.len>0){    
     PopTask(&tq,&wftemp);
    
     pthread_mutex_unlock (&tq.mut);

     if(wftemp.work!=NULL&&wftemp.args!=NULL){
      temp_args = *((function_args *) wftemp.args);
      gettimeofday(&end, NULL);
      seconds = end.tv_sec - temp_args.start.tv_sec;
      microseconds = end.tv_usec - temp_args.start.tv_usec;
      elapsed_time = seconds + microseconds / 1e6; // Convert to seconds

      pthread_mutex_lock (&tq.mut);
      delays[j] = elapsed_time;
      jobs_remaining--;
      j++;
      pthread_mutex_unlock (&tq.mut);
      
      printf("Elapsed time: %.10f seconds\n", elapsed_time);
      wftemp.work(&temp_args.val);
     pthread_cond_broadcast(&tq.notFull); 
   }	
  }
  else{
    pthread_mutex_unlock (&tq.mut);
  }
 }
 else {
   pthread_mutex_unlock (&tq.mut);
   break;
 } 
}
return (NULL);
}
void TaskQueueInit(TaskQueue *tq){
 int i=0;
 tq->len = 0;
 tq->empty = 1;
 tq->full = 0;
 tq->head = 0;
 tq->tail = 0;
 pthread_cond_init(&tq->notEmpty,NULL);
 pthread_cond_init(&tq->notFull,NULL);
 pthread_mutex_init(&tq->mut,NULL);
}
void AddTask(TaskQueue *tq, workFunction in) {
   tq->buf[tq->tail].work = in.work;
   tq->buf[tq->tail].args = in.args;
   tq->tail++;
   tq->len++;
   if (tq->tail == TASKQUEUESIZE) tq->tail = 0;
   if (tq->tail == tq->head) tq->full = 1;
   tq->empty = 0;
}
void PopTask(TaskQueue *tq, workFunction *out) {
   if(tq->len>0){
   out->work = tq->buf[tq->head].work;
   out->args = tq->buf[tq->head].args;
   tq->head++;
   tq->len--;
   if (tq->head == TASKQUEUESIZE) tq->head = 0;
   if (tq->head == tq->tail) tq->empty = 1;
   tq->full = 0;
  }
}
void calculate_cosine(int * angle){
 double radians = (*angle) * (PI / 180.0); // Convert to radians
 double cosine_value = cos(radians); // Compute cosine
 printf("Angle: %d degrees, Cosine: %f ", (*angle), cosine_value);
}