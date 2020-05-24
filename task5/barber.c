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


#define MALE_BARBERS 1
#define FEMALE_BARBERS 1
#define BOTH_BARBERS 0
#define NUM_OF_CHAIRS 5


#define NUM_OF_CLIENTS_GENERATORS 1

const key_t STATE_MUTEX_KEY =  0x1000;
const key_t BARBERS_SEMAPHORES_KEY = 0x3000;
const key_t SHARED_MEMORY_KEY = 0x6000;


void client(int ALL_BARBERS, int barberIndex);
void barber(int barberType, int ALL_BARBERS, int barberIndex);
void lock(int semid, int idx);
void unlock(int semid, int idx);
void barbersPrepareForWork(int i, int ALL_BARBERS);
void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList);



struct shared_mem {
	int maleClients;	 // 1
	int femaleClients;	 // 2
	int femaleBlockedBarbers;
	int maleBlockedBarbers;
	int anyBlockedBarbers;
} *shm;

int main() {
int ALL_BARBERS = MALE_BARBERS + FEMALE_BARBERS + BOTH_BARBERS;
pid_t processesList[ALL_BARBERS];


	int shm_id = shmget(SHARED_MEMORY_KEY, sizeof(struct shared_mem), 0666 | IPC_CREAT);
	if(shm_id < 0) {
		perror("shmget: shared memory not created");
		return 1;
	}

	//attach memory
	shm = shmat(shm_id, NULL, 0);
	if(shm == (void*)-1) {
		perror("shmat: failed to attach");
		return 1;
	} 
	
	shm->maleClients = 1;
	shm->femaleClients = 1;
	shm->femaleBlockedBarbers = 0;
	shm->maleBlockedBarbers = 0;
	shm->anyBlockedBarbers = 0;	
	
	printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients);

	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666 | IPC_CREAT);
	if(state_mutex < 0) {
		perror("semget: state mutex not created");
		return 1;
	}
	
	
	union semaphore_un{
		int val;
		unsigned int *array;
	} sem_un;

	//init mutex counter
	sem_un.val = 1;
	if(semctl(state_mutex, 0, SETVAL, sem_un) < 0) {
		perror("semctl: state mutex value failed to set");
		return 1;
	}
	

	int barber_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS + 1, 0666 | IPC_CREAT);
	if(barber_sems < 0) {
		perror("semget: barbers semaphores not created");
		return 1;
	}

	//init semaphores
	unsigned int zeros[ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS];
	for(int i = 0; i < ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS; i++) {
		zeros[i] = 0; 
	}
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

    sleep(50);

    terminate(ALL_BARBERS, processesList);
    return 0;
   
}

void barbersPrepareForWork(int i, int ALL_BARBERS)
{
	if(i < MALE_BARBERS)
	{
		printf("Male Barber created: [pid]: %d\n", getpid());	
		barber(1, ALL_BARBERS, i);
	}
	else if(i >= MALE_BARBERS && i < (FEMALE_BARBERS + MALE_BARBERS))
	{
		printf("Female Barber created: [pid] %d\n", getpid());	
		barber(2, ALL_BARBERS, i);
	}
	else if(i >= ALL_BARBERS)
	{
		printf("Process for client generation created: [pid] %d\n", getpid());
		client(ALL_BARBERS, i);
	}
	else
	{
		printf("Both Barber created: [pid] %d\n", getpid());
		barber(3, ALL_BARBERS, i);
	}
}

void barber(int barberType, int ALL_BARBERS, int barberIndex) {
   int worktime;
   int whichSexClients;
   while(1) {
	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666);
	int barbers_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS, 0666);
	if(state_mutex < 0 || barbers_sems < 0) {
		perror("grab_forks: error");
		exit(1);
	}

	lock(state_mutex, 0);
	
	switch(barberType)
	{
	   case 1:
		if(shm->maleClients > 0)
		{
	     	    shm->maleClients--;
		    worktime = (rand() % 4) + 1;
	  	    printf("Barber male [pid]: %d Cutting hair for %d seconds\n", getpid(),worktime);
		}else
		{
		    shm->maleBlockedBarbers++;
		    unlock(state_mutex, 0);
		    printf("Male barber goes sleep index: %d\n", barberIndex);
		    lock(barbers_sems, barberIndex);
		}
		break;
	   case 2:
		if(shm->femaleClients > 0)
		{
	     	    shm->femaleClients--;
		    worktime = (rand() % 4) + 1;
	  	    printf("Barber female [pid]: %d Cutting hair for %d seconds\n", getpid(),worktime);
		}else
		{
		    shm->femaleBlockedBarbers++;
		    unlock(state_mutex, 0);
		    printf("Female barber goes sleep index: %d\n", barberIndex);
		    lock(barbers_sems, barberIndex);
		}
		break;
	   case 3:
		whichSexClients = (rand() % 2) + 1;
		if(whichSexClients == 1 && shm->maleClients > 0)
		{
		   shm->maleClients--;
		   worktime = (rand() % 4) + 1;
	  	   printf("Barber any male [pid]: %d Cutting hair for %d seconds\n", getpid(),worktime);
		   break;	
		}else if(whichSexClients == 2 && shm->femaleClients > 0)
		{
		   shm->femaleClients--;
		   worktime = (rand() % 4) + 1;
	  	   printf("Barber any female [pid]: %d Cutting hair for %d seconds\n", getpid(),worktime);
		   break;	
		}else
		{
		    shm->anyBlockedBarbers++;
		    unlock(state_mutex, 0);
		    printf("Any barber goes sleep [pid]: %d\n", getpid());
		    lock(barbers_sems, barberIndex);
		}
	}
     unlock(state_mutex, 0);

	  printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients);
	  sleep(worktime);
    
	  
     
    } 
}

void client(int ALL_BARBERS, int barberIndex) {
   int waittime;
   srand(time(0)); 
   while(1) {
	int state_mutex = semget(STATE_MUTEX_KEY, 1, 0666);
	int barbers_sems = semget(BARBERS_SEMAPHORES_KEY, ALL_BARBERS + NUM_OF_CLIENTS_GENERATORS, 0666);
	if(state_mutex < 0 || barbers_sems < 0) {
		perror("grab_forks: error");
		exit(1);
	}

	lock(state_mutex, 0);
	int clientNum = (rand() % 2) + 1;	// 1 - male, 2 - female	

	  if((shm->maleClients + shm->femaleClients) > NUM_OF_CHAIRS){
		// klient nie czeka w kolejce
	  }else if(clientNum == 1)
	  {
		printf("New male client came!!\n");

		if(shm->maleClients == 0 && shm->maleBlockedBarbers > 0)
	  	{
		  printf("Male barber wake up: %d\n", shm->maleBlockedBarbers - 1);
		  unlock(barbers_sems, shm->maleBlockedBarbers - 1);
		  shm->maleBlockedBarbers--; 
		  shm->maleClients++;
	 	}else
		{
		  shm->maleClients++;
		}
	  }else if(clientNum == 2)
	  {
		printf("New female client came!!\n");

		if(shm->femaleClients == 0 && shm->femaleBlockedBarbers > 0)
	  	{
		  printf("Female barber wake up: %d\n", shm->femaleBlockedBarbers);
		  shm->femaleClients++;
		  unlock(barbers_sems, shm->femaleBlockedBarbers + MALE_BARBERS - 1);
		  shm->femaleBlockedBarbers--; 
	 	}else
		{
		  shm->femaleClients++;
		}
	    
		
	  }
	  	
	printf("People in queue: male - %d, female - %d\n", shm->maleClients, shm->femaleClients);

	unlock(state_mutex, 0);
      /* generate random number, waittime, for length of wait until next haircut or next try.  Max value from command line. */
	  waittime = (rand() % 4) + 1;
      /* sleep for waittime seconds */
	  sleep(waittime);
     }
}

void lock(int semid, int idx) 
{
	struct sembuf p = {idx, -1, SEM_UNDO};
	
	if(semop(semid, &p, 1) < 0)
	{
		perror("semop lock");
		exit(1);
	}
}

void unlock(int semid, int idx) 
{
	struct sembuf v = {idx, +1, SEM_UNDO};

	if(semop(semid, &v, 1) < 0)
	{
		perror("semop unlock");
		exit(1);
	}
}

void terminate(unsigned int indexOfLastCreatedProcess, pid_t *processesList) 
{
	for(int i = indexOfLastCreatedProcess; i >= 0; i--) 
	{
		kill(processesList[i], SIGTERM);
	}
}



