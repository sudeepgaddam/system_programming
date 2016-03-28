#include <sys/wait.h>
#include <iostream>
#include <cstdlib>
#include "fstream"
#include <map>
#include <unistd.h>
#include<stdio.h>
#include<semaphore.h>
#include<pthread.h>
#include <sys/mman.h>
#include "philo.h"
#include <fcntl.h>
 
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;


//Globals
int N,M;



semaphore_t *
semaphore_open(char *semaphore_name)
{
    int fd;
    semaphore_t *semap;


    fd = open(semaphore_name, O_RDWR, 0666);
    if (fd < 0)
        return (NULL);
    semap = (semaphore_t *) mmap(NULL, sizeof(semaphore_t),
            PROT_READ | PROT_WRITE, MAP_SHARED,
            fd, 0);
    close (fd);
    return (semap);
}


void
semaphore_post(semaphore_t *semap)
{
    pthread_mutex_lock(&semap->lock);
    if (semap->count == 0)
        pthread_cond_signal(&semap->nonzero);
    semap->count++;
    pthread_mutex_unlock(&semap->lock);
}


void
semaphore_wait(semaphore_t *semap)
{
    pthread_mutex_lock(&semap->lock);
    while (semap->count == 0)
        pthread_cond_wait(&semap->nonzero, &semap->lock);
    semap->count--;
    pthread_mutex_unlock(&semap->lock);
}

void
semaphore_fork_post(semaphore_t *semap, int num)
{
    pthread_mutex_lock(&(semap->fork_lock[num]));
    if (semap->fork_count[num] == 0)
        pthread_cond_signal(&(semap->fork_nonzero[num]));
    semap->fork_count[num]++;
    pthread_mutex_unlock(&(semap->fork_lock[num]));
}



void
semaphore_fork_wait(semaphore_t *semap,int num)
{
    pthread_mutex_lock(&(semap->fork_lock[num]));
    while (semap->fork_count[num] == 0)
    pthread_cond_wait(&(semap->fork_nonzero[num]), &(semap->fork_lock[num]));
    semap->fork_count[num]--;
    pthread_mutex_unlock(&(semap->fork_lock[num]));
}

void
semaphore_close(semaphore_t *semap)
{
    munmap((void *) semap, sizeof(semaphore_t));
}

void test(semaphore_t *semap,int ph_num){
    if (semap->state[ph_num] == HUNGRY && semap->state[LEFT(ph_num)] != EATING && semap->state[RIGHT(ph_num)] != EATING){
        semap->state[ph_num] = EATING;
        //sleep(2);
		printf("Philosopher %d takes fork %d and %d\n",ph_num+1,LEFT(ph_num)+1,ph_num+1);
		printf("Philosopher %d is Eating\n",ph_num+1);
		usleep( 1000L * rand()%( MeanEatTime ) );
        semaphore_fork_post(semap,ph_num);
    }
}

void take_fork(semaphore_t *semap ,int ph_num){
    semaphore_wait(semap);
    semap->state[ph_num] = HUNGRY;
	printf("Philosopher %d is Hungry\n",ph_num+1);
    test(semap, ph_num);
    semaphore_post(semap);
    semaphore_fork_wait(semap,ph_num);
}
 
 

 
 
void put_fork(semaphore_t *semap, int ph_num){
    semaphore_wait(semap);
    semap->state[ph_num] = THINKING;
	printf("Philosopher %d putting fork %d and %d down\n",ph_num+1,LEFT(ph_num)+1,ph_num+1);
	printf("Philosopher %d is thinking\n",ph_num+1);
	usleep( 1000L * rand() % ( MeanThinkTime ) );
    test(semap, LEFT(ph_num));
    test(semap, RIGHT(ph_num));
    semaphore_post(semap);
}




int main(int argc, char *argv[]){
	
	
	semaphore_t *semap;
	semap = semaphore_open("./backfile");

	//M-iterations, p_num-process's number
	M = atoi(argv[1]);
	N = atoi(argv[3]);
	int p_num = atoi(argv[2]);
	cout<<"Entered Philosopher " << p_num+1<<endl;

	pthread_barrier_wait (&semap->barrier);
	//cout << "N: " << N <<"," << "p_num: " << p_num <<","<< "Left: " << LEFT(p_num) << "," << "Right: "<< RIGHT(p_num) << endl;
	cout<<"Philosopher " << p_num+1 << " is thinking"<<endl;

	
	for(int i=0;i<M;i++) {
        sleep(1);
            take_fork(semap,p_num);
        sleep(0);
            put_fork(semap,p_num);
    }    
	return 1;
}
