#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <iostream>
#include <cstdlib>
using namespace std;

int main(int argc, char *argv[]){
	pid_t mapper_id, reducer_id;
	int fd[2];
	if(argc!=2){
		cout<<"Give an arguement i.e., Input file. Exiting..." << endl;
		exit(1);
	}
	pipe(fd);
	int status;
	mapper_id = fork();
   if(mapper_id==-1) {
		cerr << "Failed to fork Mapper process" << endl;
        exit(1); 
   }
   if (mapper_id == 0) {
	   //Mapper
	   close(fd[0]);
	   dup2(fd[1], STDOUT_FILENO);
	   execlp("./mapper.out","./mapper.out",argv[1]);
	   
   } else {
		//Parent
		reducer_id = fork();
	   if(reducer_id==-1) {
			cerr << "Failed to fork Reducer process" << endl;
			exit(1); 
	   }
	   
	   if (reducer_id == 0) {
			//Reducer
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
			execlp("./reducer.out","./reducer.out",NULL);
	   
		} else {
			close(fd[0]);
			close(fd[1]);
			int wpid = wait(&status);
			if(wpid != -1)  {
				//printf("Child's exit status was %d\n", status);			
		}
     }
     
    }

	return 0;
}
