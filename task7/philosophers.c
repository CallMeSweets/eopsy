#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define N 5 
#define LEFT ( i + N - 1 ) % N 
#define RIGHT ( i + 1 ) % N

void grab_forks(int i);
void put_away_forks(int i);
void test(int i); 
void *philosopher(void *arg); // start function for threads
void think(int i); // philo think
void eat(int i); // philo eat

int state[N]; // STATE FOR EVERY PHILOSOPHER
pthread_mutex_t m;
pthread_mutex_t s[N];
pthread_t philo_tid[N];

enum state // ENUM FOR PHILOSOPHER STATE
{
    THINKING,
    EATING,
    HUNGRY
}; 

int main() { 
    int philo_id[N];
    for(int i = 0; i < N; i++) {
	state[i] = THINKING; // initialisation for THINKING
	philo_id[i] = i;	

	if (pthread_mutex_init(&s[i], NULL) != 0) { // init mutex for every thread and check if error appears
            perror("error while initializing mutex");
            exit(1);
        }
	
	if(pthread_mutex_lock(&s[i]) != 0){
	    perror("error while initializing lock");
            exit(1);
	}	
    }

    for (int i = 0; i < N; i++)
    {
        if (pthread_create(&philo_tid[i], NULL, philosopher, (void *)&philo_id[i]) != 0) // CREATE THREAD AND ASSIGNE START FUNCTION WHERE THREAD SHOULD START WORK
        {
            perror("error while creating thread");
            exit(1);
        }
    }

    sleep(30);
	
    for (int i = 0; i < N; i++)
    {
        pthread_cancel(philo_tid[i]); // cancel threads
        pthread_join(philo_tid[i], NULL); // wait for thread terminates, If thread has already terminated, then pthread_join() returns immediately
	pthread_mutex_destroy(&s[i]);
    }
    
    pthread_mutex_destroy(&m); // delete single mutex
  
} 

void *philosopher(void *arg) // start function assigned to thread
{
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
    {
        perror("error whyle setting cancel type to asynchronous");
        exit(EXIT_FAILURE);
    }
    int i = *((int *)arg);

    while (true) // infinite while loop
    {
        think(i);
        grab_forks(i);
        eat(i);
        put_away_forks(i);
    }

    return NULL;
}

void think(int i) // printf thinking info 
{
    printf("PHILOSOPHER[%d]: THINKING\n", i);
    sleep(3);
}

void eat(int i) // printf eating info
{
    printf("PHILOSOPHER[%d]: EATING\n", i);
    sleep(3);
}

void grab_forks( int i ) {
   pthread_mutex_lock(&m);  // lock critical part of code
   state[i] = HUNGRY;   
   test(i);  
   pthread_mutex_unlock(&m); // unlock critical part of code
   pthread_mutex_lock(&s[i]); 
}

 

void put_away_forks( int i ) {  
   pthread_mutex_lock(&m); // lock critical part of code
   state[i] = THINKING;   
   test(LEFT);   
   test(RIGHT);  
   pthread_mutex_unlock(&m); // unlock critical part of code
}

 

void test( int i ) {  
   if( state[i] == HUNGRY   && state[LEFT] != EATING   && state[RIGHT] != EATING )  {
      state[i] = EATING;   
      pthread_mutex_unlock(&s[i]); // unlock philo
    }
}
