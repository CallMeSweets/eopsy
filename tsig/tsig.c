#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include<sys/wait.h> 
#include <unistd.h> 
#include <signal.h>
#include <time.h>

void sendSIGTERM();
void forkChild(int i);

# define NUM_CHILD 3
pid_t childProcessTable[NUM_CHILD];
int numCreatedChilds=0;

int main() 
{ 
    printf("parent[%d]: created\n", getpid()); 
     	
 
     for(int i=0;i<NUM_CHILD;i++)
    { 
	sleep(1);
	forkChild(i);
		
    } 
	
    pid_t pidTerminatedProcess[numCreatedChilds];
    int exitCodeTerminatedProcess[numCreatedChilds];
    int exitProcesses = 0;
    int status;

    if(numCreatedChilds == NUM_CHILD)
    {
    	for(int i=0;i<NUM_CHILD;i++)
	{
    	   pidTerminatedProcess[i] = wait(&status); // It suspends execution of the calling process until one of its children terminates
	
	   if(WIFEXITED(status)) // returns true if the child terminated normally.
	   {
		exitCodeTerminatedProcess[i] = WEXITSTATUS(status); //returns the exit status of the child.
		exitProcesses++;
	   }
	} 
    }

    printf("parent[%d]: No more child to be processed\n", getpid()); 
    
    for(int i=0;i<NUM_CHILD;i++)
    {
	printf("child[%d]: terminated, exit status -> %d\n", pidTerminatedProcess[i], exitCodeTerminatedProcess[i]); 
    }

    printf("Number of processes exit codes %d: \n", exitProcesses); 

} 


void forkChild(int i) {
    pid_t childPid;

    switch (childPid = fork()) 
        {
    	    case -1:
		printf("fork() failed, child not created");
		sendSIGTERM();
		exit(1);
		break;
    	    case 0: //child
		printf("child[%d]: created and his parent[%d] \n",getpid(),getppid()); 
		printf("child[%d]: going sleep for 10s \n",getpid()); 
		sleep(10);
		printf("child[%d]: execution is completed \n",getpid());
		exit(0); 
		break;
	    default: // parent
		childProcessTable[i] = childPid;
		numCreatedChilds++;
		break;
 	}
}

void sendSIGTERM(){
    for(int i=0; i<NUM_CHILD; i++)
    {
	kill(childProcessTable[i], SIGTERM); 
    }
}
