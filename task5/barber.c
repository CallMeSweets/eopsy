#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>

struct Barber {
   int sex;
};  


#define MALE_BARBERS 5
#define FEMALE_BARBERS 5
#define BOTH_BARBERS 5
#define NUM_OF_CHAIRS 15



int createBarbersProcesses(pid_t *processesList, int allBarbersNumber);
void startWorking(int i, int ALL_BARBERS);
void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList);

int clients[NUM_OF_CHAIRS];


int main() 
{
int ALL_BARBERS = MALE_BARBERS + FEMALE_BARBERS + BOTH_BARBERS;
pid_t processAllBarbers[ALL_BARBERS];


createBarbersProcesses(processAllBarbers, ALL_BARBERS);
} 



int createBarbersProcesses(pid_t *processesList, int allBarbersNumber){
    for(int i = 0; i < allBarbersNumber+1; i++){
	pid_t pid = fork();
	
	if(pid < 0) { //process not created
		perror("Failed to create child process");
		terminate(i - 1, processesList);
		return 1;
	}
	else if(pid > 0) { //parent process
		//store process id
		processesList[i] = pid;
	}
	else {	//child barber process
		startWorking(i, ALL_BARBERS);
		return 0;
	}
    }
}

void startWorking(int i, int ALL_BARBERS){
	if(i < MALE_BARBERS){
		printf("Male Barber created: [pid]: %d\n", getpid());	
	}else if(i > MALE_BARBERS && i <= (FEMALE_BARBERS + MALE_BARBERS)){
		printf("Female Barber created: [pid] %d\n", getpid());	
	}else if(i => ALL_BARBERS){
		generateClients();
	}else{
		printf("Both Barber created: [pid] %d\n", getpid());
	}
}


void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList) {
	for(int i = indexOfLastCreatedProcess; i >= 0; i--) {
		kill(processesList[i], SIGTERM);
	}
}




