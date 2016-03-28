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
#include <sys/types.h>
#include <sys/stat.h>
#include "philo.h"
using namespace std;

semaphore_t *
semaphore_create(char *semaphore_name,int N)
{
	int fd;
    semaphore_t *semap;
    pthread_mutexattr_t psharedm;
    pthread_condattr_t psharedc;


    fd = open(semaphore_name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd < 0)
        return (NULL);
    (void) ftruncate(fd, sizeof(semaphore_t));
    (void) pthread_mutexattr_init(&psharedm);
    (void) pthread_mutexattr_setpshared(&psharedm,
        PTHREAD_PROCESS_SHARED);
    (void) pthread_condattr_init(&psharedc);
    (void) pthread_condattr_setpshared(&psharedc,
        PTHREAD_PROCESS_SHARED);
        
        
    semap = (semaphore_t *) mmap(NULL, sizeof(semaphore_t),
            PROT_READ | PROT_WRITE, MAP_SHARED,
            fd, 0);
 
            
    close (fd);
    
               
    for(int i=0;i<N;i++) {
		(void) pthread_mutex_init(&semap->fork_lock[i], &psharedm);
		(void) pthread_cond_init(&semap->fork_nonzero[i], &psharedc);

		semap->fork_count[i] = 0;
	} 
	
    (void) pthread_mutex_init(&semap->lock, &psharedm);
    (void) pthread_cond_init(&semap->nonzero, &psharedc);
    pthread_barrierattr_t barattr;

    pthread_barrierattr_setpshared(&barattr, PTHREAD_PROCESS_SHARED);

    pthread_barrier_init (&semap->barrier, &barattr, N);
    semap->count = 1;
    
    
    return (semap);
}

//arg1: number of philosophers
//arg2: M, philosopher iterations
int main( int argc, char *argv[] )
{
	pid_t mapper_id, reducer_id;
	int fd[2];
	if(argc<3){
		cout<<"Give two arguements i.e., <#philosphers> <#iterations>" << endl;
		exit(1);
	}
	
	int N = atoi(argv[1]);
	int M = atoi(argv[2]);
	cout << N <<endl;
	pid_t pid;
	//vector<pid_t> philosophers(N); 
	int status;
	int wpid;
	
    semaphore_t *semap;
    remove("./backfile");
	semap = semaphore_create("./backfile", N);
	if (semap == NULL)
	exit(1);
	char str[8];
	for(int i=0;i<N;i++)
	 {
	   int aInt=i;
	   sprintf(str, "%d",i);
	   pid=fork();
	   if(pid==0)
	   {
	   execlp("./philosopher.out", "./philosopher.out",argv[2], &str,argv[1], NULL);
	   perror("execve");   /* execve() returns only on error */
	   exit(EXIT_FAILURE);
	   }
	 }
	while ((wpid = wait(&status)) > 0); 
	cout<<"------------------------" << endl;
	cout<<"Dining Philosopers done" <<endl;
   return 0;
}
