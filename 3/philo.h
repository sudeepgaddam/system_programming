#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <sys/wait.h>
#include<stdio.h>
#include<semaphore.h>
#include<pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */ 
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

#define LEFT(i) (i+(N-1))%N
#define RIGHT(i) (i+1)%(N)
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define MAX 1000
const int MeanThinkTime		= 1000;	// average think time in milliseconds
const int MeanEatTime		= 750;	// average eating time in milliseconds

struct semaphore {
    pthread_mutex_t lock;
    pthread_mutex_t fork_lock[MAX];
    pthread_cond_t nonzero;
    pthread_barrier_t   barrier;
    unsigned count;
    pthread_cond_t fork_nonzero[MAX];
    unsigned fork_count[MAX];
    int state[MAX];


};
typedef struct semaphore semaphore_t;

semaphore_t *semaphore_create(char *semaphore_name,int N);
semaphore_t *semaphore_open(char *semaphore_name);
void semaphore_post(semaphore_t *semap);
void semaphore_wait(semaphore_t *semap);
void semaphore_close(semaphore_t *semap);

