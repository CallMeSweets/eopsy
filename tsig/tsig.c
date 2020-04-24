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
void onClick(int signal); // handler on interrupt
void terminateChild(int signal); // handler for child on SIGTERM signal
char interruptFlag = 0;
# endif

void sendSIGTERM(); // function to kill all childs processess
void forkChild(int i); // function to create child

pid_t childProcessTable[NUM_CHILD]; // table for childs
int numCreatedChilds = 0; // number of created childs

int main() 
{ 
    printf("parent[%d]: created\n", getpid()); 
     	
 
     for(int i=0;i<NUM_CHILD;i++) //loop to create childs
    { 
	#ifdef WITH_SIGNALS
	   for(int j=1;j<NSIG; j++)
 	   {
		signal(j, SIG_IGN);  // send SIG_IGN to child, to ignore INTERRUPT, SIGINT means INTERRUPT
	   } 	
	   signal(SIGCHLD, SIG_DFL); // set default signal configuration for child
	   signal(SIGINT, onClick); // sign my function as reaction for INTERRUPT
	#endif
	
	sleep(1);
	forkChild(i); // function to create child

	#ifdef WITH_SIGNALS
	   if(interruptFlag == 1) // check if interrupt was clicked
	     {
	      printf("parent[%d]: keyboard interrupt during creation process\n", getpid());
	      sendSIGTERM(); // function which send signal about interrupt to all created childs
  	      break;
	     }
	#endif
		
    } 
	
    pid_t pidTerminatedProcess[numCreatedChilds]; //  table for terminated childs
    int exitCodeTerminatedProcess[numCreatedChilds]; // table for exit status of all childs
    int exitProcesses = 0; // number of correct terminated childs
    int status;

  
	for(int i=0;i<numCreatedChilds;i++)
	{
	   pidTerminatedProcess[i] = wait(&status); // It suspends execution of the calling process until one of its children terminates

	   if(WIFEXITED(status)) // returns true if the child terminated normally.
	   {
		exitCodeTerminatedProcess[i] = WEXITSTATUS(status); //returns the exit status of the child.
		exitProcesses++;
	   }
	} 
    

    printf("parent[%d]: No more child to be processed\n", getpid()); 
    
    for(int i=0;i<numCreatedChilds;i++) // loop to printf info
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


void forkChild(int i) { // function to create child
    pid_t childPid;

    switch (childPid = fork()) // create child
        {
    	    case -1: // if negative value then was a fail during child creation
		printf("fork() failed, child not created\n");
		sendSIGTERM(); // kill all created childs
		exit(1);
		break;
    	    case 0: //value when child was created	
	
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
	    default: // is 1 when parent was created, but default take it into account
		childProcessTable[i] = childPid; // add created child process to the table
		numCreatedChilds++; // increase number of created childs
		break;
 	}
}

void sendSIGTERM(){ // function to kill all childs
    for(int i=0; i<numCreatedChilds; i++) // loop to kill all childs
    {
	printf("parent[%d] sending SIGTERM to child[%d] \n", getpid(), childProcessTable[i]);
	kill(childProcessTable[i], SIGTERM);  // send termination singal to child
    }
}


#ifdef WITH_SIGNALS
void onClick(int signal) // handler on interrupt
{
    printf("parent[%d]: interrupt click \n", getpid());
    interruptFlag = 1; // if interrupt was clicked then change flag
}

void terminateChild(int signal) // handler on child SIGTERM signal
{
	printf("child[%d]: process terminated\n", (int)getpid());
	exit(1); // terminate child process
}
#endif
