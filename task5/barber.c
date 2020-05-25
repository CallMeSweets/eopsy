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
#include<time.h>


#define MALE_BARBERS 2
#define FEMALE_BARBERS 2
#define BOTH_BARBERS 3
#define NUM_OF_CHAIRS 20


#define NUM_OF_CLIENTS_GENERATORS 6

const key_t STATE_MUTEX_KEY =  0x1000;	// unique key for mutex semaphore
const key_t BARBERS_SEMAPHORES_KEY = 0x6060; // unique key for semaphores for barbers
const key_t SHARED_MEMORY_KEY = 0x2060;		// unique key for shared memory semaphore


void client(int ALL_BARBERS, int barberIndex);	// function symulate client comes
void barber(int barberType, int ALL_BARBERS, int barberIndex); // function symulate barber work
void lock(int semid, int idx);	// lock ciritcal part of code
void unlock(int semid, int idx); // unlock ciritcal part of code
void barbersPrepareForWork(int i, int ALL_BARBERS); // distribution of barbers for different types
void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList); // killing all processes
void moveBarbersIndexes(int blockedBarbers[], int size); // remove barber from array which contains blockedBarbers == sleeping barbers
int getFirstFreeIndex(int blockedBarbers[], int size);   // get first avaliable index where index of barber which goes sleep can be save

// struct which is shared memory between all processes

struct shared_mem {
	int maleClients;	 // number of maleClients in queue
	int femaleClients;	 // number of femaleClients in queue
	int femaleBlockedBarbers[FEMALE_BARBERS + 1]; // array for blocked female barbers processes, later initlize with -1
	int maleBlockedBarbers[MALE_BARBERS + 1];	// array for blocked male barbers processes, later initlize with -1
	int anyBlockedBarbers[BOTH_BARBERS + 1];	// array for blocked female+male barbers processes, later initlize with -1
} *shm;

int main() {
int ALL_BARBERS = MALE_BARBERS + FEMALE_BARBERS + BOTH_BARBERS;
pid_t processesList[ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS]; // array for all barbers processes and clients generators processes

// create semaphore for shared memory
	int shm_id = shmget(SHARED_MEMORY_KEY, sizeof(struct shared_mem), 0666 | IPC_CREAT); 
	if(shm_id < 0) {
		perror("shmget: shared memory not created");
		return 1;
	}

	//attach memory
	shm = shmat(shm_id, NULL, 0);
	if(shm == (void*)-1) { 		//check if correctly attached
		perror("shmat: failed to attach");
		return 1;
	} 
	
// some start value initialization

	shm->maleClients = 3;
	shm->femaleClients = 3;
	for(int i = 0; i < FEMALE_BARBERS; i++)
		shm->femaleBlockedBarbers[i] = -1;
	for(int i = 0; i < MALE_BARBERS; i++)
		shm->maleBlockedBarbers[i] = -1;
	for(int i = 0; i < BOTH_BARBERS; i++)
		shm->anyBlockedBarbers[i] = -1;

// end of value initialization
	
	printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients);

// create mutex, mutex is binary semaphore with access 0/1
// mutex will lock/unlock to part of code
	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666 | IPC_CREAT);
	if(state_mutex < 0) {		// check if correctly created
		perror("semget: state mutex not created");
		return 1;
	}
	
// created union for mutex
	union semaphore_un{
		int val;
		unsigned int *array;
	} sem_un;

	//init mutex counter
	sem_un.val = 1; 	// set access
	if(semctl(state_mutex, 0, SETVAL, sem_un) < 0) {
		perror("semctl: state mutex value failed to set");
		return 1;
	}
	
// created semaphore which wil allow me to stop/start given barber processes
// client generator processes will run constantly 
	int barber_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS, 0666 | IPC_CREAT);
	if(barber_sems < 0) {
		perror("semget: barbers semaphores not created");
		return 1;
	}

	//init semaphores for all barbers
	unsigned int zeros[ALL_BARBERS];
	for(int i = 0; i < ALL_BARBERS; i++) {
		zeros[i] = 0; 
	}

// for start set all barbers in unlock state
	sem_un.array = zeros;
	if(semctl(barber_sems, 0, SETALL, sem_un) < 0) {
		perror("semctl: barber semaphores values failed to set");
		return 1;
	}

	//barber creation and client generator creation
	for(int i = 0; i < ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS; i++){
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
		barbersPrepareForWork(i, ALL_BARBERS);
		return 0;
	}
    }

    sleep(60); // finish program after that time

// kill all processes
    terminate(ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS - 1, processesList);
    return 0;
   
}

void barbersPrepareForWork(int i, int ALL_BARBERS)
{
	if(i < MALE_BARBERS) // the barbers from 0 to MALE_BARBERS are created as maleBarbers
	{
		printf("CREATED Male Barber [pid]: %d\n", getpid());	
		barber(1, ALL_BARBERS, i); // run barber function
	}
	else if(i >= MALE_BARBERS && i < (FEMALE_BARBERS + MALE_BARBERS)) // another interval now for femaleBarbers creation
	{
		printf("CREATED Female Barber [pid] %d\n", getpid());	
		barber(2, ALL_BARBERS, i);	// run barber function
	}
	else if(i >= ALL_BARBERS)
	{
	// for index > ALL_BARBERS are clients processes created
		printf("CREATED Process for client generation [pid] %d\n", getpid()); 
		client(ALL_BARBERS, i);  // run client function
	}
	else
	{
		printf("CREATED Any Barber [pid] %d\n", getpid()); // another cases are both type barbers created
		barber(3, ALL_BARBERS, i); // run barber function
	}
}

void barber(int barberType, int ALL_BARBERS, int barberIndex) {
   int worktime; // barber work time
   srand(time(0)); // random generator
   while(1) {	// infite loop
	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666);  // get state_mutex for lock/unlock critical part of code
	int barbers_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS, 0666); // get semaphore for stop/start barber process
	if(state_mutex < 0 || barbers_sems < 0) { // check if correctly created
		perror("grab_forks: error");
		exit(1);
	}

	lock(state_mutex, 0); // lock for critical part of code, the shared memory modification
	
	switch(barberType) // check what type of barber it is: 1 - Male, 2 - Female, 3 - Both
	{
	   case 1:
		if(shm->maleClients > 0) // if there are some male clients take him
		{
	     	    shm->maleClients--; // client was taken
		    worktime = (rand() % 4) + 1; // rand time of work with client
	  	    printf("CUTTING HAIR Barber male [pid]: %d, seconds: %d\n", getpid(),worktime); // info about cutting
		}else
		{
		// if there is no client then male barber goes sleep
		// the index of sleeping male barber is saved in array
		    shm->maleBlockedBarbers[getFirstFreeIndex(shm->maleBlockedBarbers, MALE_BARBERS)] = barberIndex;
		    printf("SLEEP Male barber index: %d\n", barberIndex); // printf info
		    unlock(state_mutex, 0);	// here have to be unlock before the proccess will go sleep
		    lock(barbers_sems, barberIndex);	// barber process go sleep
		}
		break;
	   case 2:
		if(shm->femaleClients > 0) // if there are some female clients take him
		{
	     	    shm->femaleClients--;	// client was taken
		    worktime = (rand() % 4) + 1;	// rand time of work with client
	  	    printf("CUTTING HAIR Barber female [pid]: %d, seconds: %d\n", getpid(),worktime);  // info about cutting
		}else
		{
		// if there is no client then female barber goes sleep
		// the index of sleeping female barber is saved in array
		    shm->femaleBlockedBarbers[getFirstFreeIndex(shm->femaleBlockedBarbers, FEMALE_BARBERS)] = barberIndex;
		    printf("SLEEP Female barber index: %d\n", barberIndex);	// printf info
		    unlock(state_mutex, 0);	// here have to be unlock before the proccess will go sleep
		    lock(barbers_sems, barberIndex);	// barber process go sleep
		}
		break;
	   case 3:	// male+female barber 
		if(shm->maleClients > shm->femaleClients) // if there is more male clients -> take male client
		{
		   shm->maleClients--; 		// take male client
		   worktime = (rand() % 4) + 1;	// rand work time
	  	   printf("CUTTING HAIR Barber any male [pid]: %d, seconds: %d\n", getpid(),worktime);	// printf info
		}else if(shm->maleClients <= shm->femaleClients) // if there is more female clients -> take female client
		{
		   shm->femaleClients--;	// take female client
		   worktime = (rand() % 4) + 1;	// rand work time
	  	   printf("CUTTING HAIR Barber any female [pid]: %d, seconds: %d\n", getpid(),worktime); // printf info	
		}else if(shm->femaleClients == 0 && shm->maleClients == 0) // if no client go sleep
		{
		// the index of sleeping female+male barber is saved in array
		    shm->anyBlockedBarbers[getFirstFreeIndex(shm->anyBlockedBarbers, BOTH_BARBERS)] = barberIndex;
		    printf("SLEEP Any barber index: %d\n", barberIndex); // printf info
		    unlock(state_mutex, 0);	// unlock critical part of code before sleep
		    lock(barbers_sems, barberIndex); // sleep barber process
		}
	    	break;
	}
     printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients); // print info
     unlock(state_mutex, 0); 
// unlock critical part of code has finished
	  
     sleep(worktime); // process stop -> it is work time on client
    
	  
     
    } 
}

void client(int ALL_BARBERS, int barberIndex) {
   int waittime;	// barber work time
   srand(time(0)); 	// // random generator
   while(1) { // infinite loop
	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666);	// get state_mutex for lock/unlock critical part of code
	int barbers_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS, 0666);	// get semaphore for stop/start barber process
	if(state_mutex < 0 || barbers_sems < 0) {	// check if correctly created
		perror("grab_forks: error");
		exit(1);
	}

	lock(state_mutex, 0);	// lock for critical part of code, the shared memory modification
	int clientNum = (rand() % 2) + 1;	// 1 - male, 2 - female, random for client generation

	  if((shm->maleClients + shm->femaleClients) >= NUM_OF_CHAIRS){
		// client does no wait in queue
	  }else if(clientNum == 1)
	  {
		printf("New male client came!!\n"); // info about male client came
		// if there are some sleeping male barbers wake up one of them
		if(shm->maleClients >= 0 && shm->maleBlockedBarbers[0] > -1 && MALE_BARBERS >= 1)
	  	{
		  printf("WAKE UP Male barber: %d\n", shm->maleBlockedBarbers[0]); // barber info wake up
		  unlock(barbers_sems, shm->maleBlockedBarbers[0]); // unlock barber process
		  moveBarbersIndexes(shm->maleBlockedBarbers, MALE_BARBERS); // remove barber index from blockedBarbers table
	 	}
		// if there are some female+male sleeping barbers wake up one of them
		else if(shm->maleClients >= 0 && shm->anyBlockedBarbers[0] > -1 && BOTH_BARBERS >= 1){
		  printf("WAKE UP Any barber: %d\n", shm->anyBlockedBarbers[0]); // barber info wake up
		  unlock(barbers_sems, shm->anyBlockedBarbers[0]); // unlock barber process
		  moveBarbersIndexes(shm->anyBlockedBarbers, BOTH_BARBERS); // remove barber index from blockedBarbers table
		}

		shm->maleClients++; // increase number of male clients
	     

	  }else if(clientNum == 2)
	  {
		printf("New female client came!!\n");	// info about male client came
		// if there are some female sleeping barbers wake up one of them
		if(shm->femaleClients >= 0 && shm->femaleBlockedBarbers[0] > -1 && FEMALE_BARBERS >= 1)
	  	{
		  printf("WAKE UP Female barber: %d\n", shm->femaleBlockedBarbers[0]); // barber info wake up
		  unlock(barbers_sems, shm->femaleBlockedBarbers[0]);	// unlock barber process
		  moveBarbersIndexes(shm->femaleBlockedBarbers, FEMALE_BARBERS);  // remove barber index from blockedBarbers table
	 	}
		// if there are some female+male sleeping barbers wake up one of them
		else if(shm->femaleClients >= 0 && shm->anyBlockedBarbers[0] > -1 && BOTH_BARBERS >= 1){
		  printf("WAKE UP Any barber: %d\n", shm->anyBlockedBarbers[0]); // barber info wake up
		  unlock(barbers_sems, shm->anyBlockedBarbers[0]); // unlock barber process
		  moveBarbersIndexes(shm->anyBlockedBarbers, BOTH_BARBERS); // remove barber index from blockedBarbers table
		}

		shm->femaleClients++; // increase number of female clients
	    
	    	
	  }
	  	
	printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients);
	// unlock critical part of code
	unlock(state_mutex, 0);
      	// some random time before next client come
	  waittime = (rand() % 4) + 1;
        // wait that time
	  sleep(waittime);
     }
}

//function for lock given semaphore
void lock(int semid, int idx) 
{
	struct sembuf p = {idx, -1, SEM_UNDO};
	
	if(semop(semid, &p, 1) < 0)
	{
		perror("semop lock");
		exit(1);
	}
}

//function for unlock given semaphore
void unlock(int semid, int idx) 
{
	struct sembuf v = {idx, +1, SEM_UNDO};

	if(semop(semid, &v, 1) < 0)
	{
		perror("semop unlock");
		exit(1);
	}
}

// function for killing all processes
void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList) 
{
	for(int i = indexOfLastCreatedProcess; i >= 0; i--) 
	{
		kill(processesList[i], SIGTERM);
	}
}

// when barber wake up, remove him from array of blockedBarbers==sleeping barbers
void moveBarbersIndexes(int blockedBarbers[], int size){
	for(int i = 0; i < size-1; i++)
	{
	   blockedBarbers[i] = blockedBarbers[i+1];
	}
}


// value -1 means that there is no index of sleeping barber
// get first index when that value appears to save there the barber index which goes sleep
int getFirstFreeIndex(int blockedBarbers[], int size)
{
	for(int i = 0; i < size-1; i++)
	{
	   if(blockedBarbers[i] == -1)
		return i;
	}
}


