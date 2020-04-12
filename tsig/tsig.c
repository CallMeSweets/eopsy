#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include<sys/wait.h> 
#include <unistd.h> 
#include <signal.h>
#include <time.h>



# define WITH_SIGNALS
# define NUM_CHILD 10

# ifdef WITH_SIGNALS
void onClick(int signal);
void terminateChild(int signal);
char interruptFlag = 0;
# endif

void sendSIGTERM();
void forkChild(int i);

pid_t childProcessTable[NUM_CHILD];
int numCreatedChilds = 0;

int main() 
{ 
    printf("parent[%d]: created\n", getpid()); 
     	
 
     for(int i=0;i<NUM_CHILD;i++)
    { 
	#ifdef WITH_SIGNALS
	   for(int j=0;j<NSIG; j++)
 	   {
		signal(SIGINT, SIG_IGN);
	   } 	
	   signal(SIGCHLD, SIG_DFL);
	   signal(SIGINT, onClick);
	#endif
	
	sleep(1);
	forkChild(i);

	#ifdef WITH_SIGNALS
	   if(interruptFlag == 1)
	     {
	      printf("parent[%d]: keyboard interrupt during creation process\n", getpid());
	      sendSIGTERM();
  	      break;
	     }
	#endif
		
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

    #ifdef WITH_SIGNALS //restore deafult signal handler
	for(int i=1; i<NSIG; i++)
	{
	    signal(i, SIG_DFL);
	}
    #endif
    

} 


void forkChild(int i) {
    pid_t childPid;

    switch (childPid = fork()) 
        {
    	    case -1:
		printf("fork() failed, child not created\n");
		sendSIGTERM();
		exit(1);
		break;
    	    case 0: //child	
	
		#ifdef WITH_SIGNALS
		signal(SIGINT, SIG_IGN); //ingore interrupt 
		signal(SIGTERM, terminateChild); // add handler for SIGTERM
		#endif

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
	printf("parent[%d] sending SIGTERM to child[%d] \n", getpid(), childProcessTable[i]);
	kill(childProcessTable[i], SIGTERM); 
    }
}


#ifdef WITH_SIGNALS
void onClick(int signal)
{
    printf("parent[%d]: interrupt click \n", getpid());
    interruptFlag = 1; 
}

void terminateChild(int signal)
{
	printf("child[%d]: process terminated\n", (int)getpid());
	exit(1);
}
#endif
