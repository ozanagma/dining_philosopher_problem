#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#define TRUE 1
#define FALSE 0
#define MAX_SLEEP_TIME_USEC 1000000
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct{
    int eat_cnt;
    int think_cnt;
    int sing_cnt;
    long long sing_time_sec;
}S_PHILOSOPHER_DATA;

int philosopher_count;
S_PHILOSOPHER_DATA* phil_data;
sem_t* forks;
sem_t  microphone;

void sigint_handler(int signum);
void* philosopher_func(void* param);
void rand_usleep(int max_time);
void (*eat)(int)    = rand_usleep;
void (*think)(int)  = rand_usleep;
void (*sing)(int)   = rand_usleep;

int main(int argc, char* argv[])
{
    signal(SIGINT, sigint_handler); // Ctrl + C handler

    if(argc == 1){
        printf("Please enter philosopher count.\n");
        exit(1);
    }
    else if( argc > 2){
        printf("Too many arguments.\n");
        exit(1);
    }

    philosopher_count = atoi(argv[1]);

    time_t t;
    srand((unsigned)time(&t));

    phil_data                   = (S_PHILOSOPHER_DATA*)malloc(philosopher_count * sizeof(S_PHILOSOPHER_DATA));
    forks                       = (sem_t*)malloc(philosopher_count * sizeof(sem_t));
    pthread_t* phil_thread_ids  = (pthread_t*)malloc(philosopher_count * sizeof(pthread_t));
    int* thread_id              = (int*)malloc(philosopher_count * sizeof(int));
    
    for(int i = 0; i < philosopher_count; i++)
        sem_init(&forks[i], 0, 1);
    
    sem_init(&microphone, 0, 1);

    for(int i = 0; i < philosopher_count; i++){
        thread_id[i] = i;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&phil_thread_ids[i], &attr, philosopher_func, &thread_id[i]);
    } 

    for(int i = 0; i < philosopher_count; i++)
        pthread_join(phil_thread_ids[i], NULL);

    return 0;
}

void rand_usleep(int max_time)
{
    usleep(rand() % max_time);
}

void* philosopher_func(void* param)
{
    int philosopher_id = *(int*)param;
    struct timeval sing_start_t, sing_stop_t;
    double sing_t_sec;
    int min_fork_id = MIN(philosopher_id, (philosopher_id + 1) % philosopher_count);
    int max_fork_id = MAX(philosopher_id, (philosopher_id + 1) % philosopher_count);

    while(TRUE){

        sem_wait(&forks[min_fork_id]);
        //printf("%d is get fork %d.\n", philosopher_id, min_fork_id);
        sem_wait(&forks[max_fork_id]);
        //printf("%d is get fork %d.\n", philosopher_id, max_fork_id);
        printf("Philosopher %d is eating.\n",   philosopher_id);
        eat(MAX_SLEEP_TIME_USEC);
        phil_data[philosopher_id].eat_cnt++;
        sem_post(&forks[min_fork_id]);
        //printf("%d is released fork %d.\n", philosopher_id, min_fork_id);
        sem_post(&forks[max_fork_id]); 
        //printf("%d is released fork %d.\n", philosopher_id, max_fork_id);


        printf("Philosopher %d is thinking.\n", philosopher_id);
        think(MAX_SLEEP_TIME_USEC);
        phil_data[philosopher_id].think_cnt++;

        if(rand()%2){
            sem_wait(&microphone);
            //printf("%d is get mic.\n", philosopher_id);
            printf("Philosopher %d is singing.\n",  philosopher_id);
            gettimeofday(&sing_start_t, NULL);
            sing(MAX_SLEEP_TIME_USEC);
            gettimeofday(&sing_stop_t, NULL);
            sem_post(&microphone);
            //printf("%d is released mic.\n", philosopher_id);
            sing_t_sec = (sing_stop_t.tv_sec - sing_start_t.tv_sec) * 1000 + (sing_stop_t.tv_usec - sing_start_t.tv_usec) / 1000;
            phil_data[philosopher_id].sing_cnt++;
            phil_data[philosopher_id].sing_time_sec += sing_t_sec;
        }

    }
}


void sigint_handler(int signum)
{
    printf(" is catched.\n");
    int headliner_id = 0;
    long long max_sing_time = 0;
    for(int i = 0; i < philosopher_count; i++){
        if(phil_data[i].sing_time_sec > max_sing_time){
            max_sing_time = phil_data[i].sing_time_sec;
            headliner_id = i;
        }
        printf("Philosopher %d -> Eat Count: %d Think Count: %d Sing Count: %d Sing Time(sec): %lld\n", 
                i, phil_data[i].eat_cnt, phil_data[i].think_cnt, phil_data[i].sing_cnt, phil_data[i].sing_time_sec );
    }
    printf("Headliner is philosopher %d with %lld singing time.\n", headliner_id, phil_data[headliner_id].sing_time_sec);
    exit(1);
}

