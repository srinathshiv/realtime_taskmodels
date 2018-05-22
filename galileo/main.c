#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include<linux/input.h>

#define MOUSEEVENT "/dev/input/event3"
#define num_mutexes 10 //There can be 10 max mutexes acquired by each thread

//pthread mutexes for resources handling
pthread_mutex_t mutex[num_mutexes];
pthread_mutexattr_t mutex_attr[num_mutexes];
pthread_mutex_t event_mutexes[2];
pthread_mutexattr_t event_mutexes_attr[2];
pthread_cond_t events[2]; //pthread events for handling blocked aperiodic tasks 

void mutex_init(); 
void Compute(int);
void Lock(int,int);
void Unlock(int);
void* P_task(void*);
void* A_task(void*);
void* dispatch();

enum body{lock,unlock,compute};//enum to determine whether a thread locks/unlocks a mutex or does a computations

enum tasktype{periodic,aperiodic};//enum for defining a thread as periodic or aperiodic

struct args{//struct for constructing an argument as a linked list
	int type_arg;
	enum body bodytype;
	int body_arg;
	struct args* next;
	struct sched_param param;
};

typedef struct args args;


struct Thread_t{
	pthread_t pid;
	args* thread_body;
	enum tasktype type;
};

typedef struct Thread_t Thread_t;

pthread_barrier_t Barrier;

int die = 0;
int trace_fd=-1;
int marker_fd=-1;

int main(int argc, char* argv[]){

	pthread_cond_init(&events[0],NULL);
	pthread_cond_init(&events[1], NULL );

	mutex_init();

	FILE *pFile = fopen(argv[1],"rb");
	if (pFile==NULL) perror ("Error opening file");
	char *line = NULL;
	size_t len = 0;
	getline(&line, &len, pFile);
	int numtasks;
	int runtime;

	int count=0;
	sscanf(line,"%d %d",&numtasks,&runtime);

	pthread_barrier_init(&Barrier,NULL,numtasks+2);
	Thread_t threads[numtasks];//tasks statically allocated based on the input file
	while(count<numtasks){
		getline(&line, &len, pFile);
		while(*line==' ') line++;
		if(*line!='/'&&*(line+1)!='/')
		{
			args* newtask = (args *)malloc(sizeof(args));

			threads[count].type= *line=='P' ? periodic : aperiodic;
			line++;
			while(*line==' ')line++;
			newtask->param.sched_priority=atoi(line); 
			while(*line!=' ')line++;
			while(*line==' ')line++;
			newtask->type_arg = atoi(line);
			while(*line!=' ') line++;
			while(*line==' ')line++;
			args* dummy=newtask;
			while(1)
			{
				if(*line=='L'||*line=='U'){
					dummy->bodytype= *line=='L'?lock: unlock;
					dummy->body_arg=atoi(line+1);
				}	
				else{
					dummy->bodytype = compute;
					dummy->body_arg = atoi(line);
				}
				int leave=0;
				dummy->next=NULL;
				while(*line!=' '){
					line++;
					if(*line=='\n'){
						leave=1;
						dummy->next=NULL;
						break;
					} 
				}
				if(leave==1) break;
				while(*line==' ')line++;
				dummy->next=(args *)malloc(sizeof(args));
				dummy = dummy->next;
			}
			threads[count++].thread_body=newtask;
			dummy = newtask;


		}
	}

	for(int i=0;i<numtasks;i++){
		if(threads[i].type==periodic)//schedules based on what was parsed from the input file 
			pthread_create(&threads[i].pid,NULL,P_task,threads[i].thread_body);
		else 
			pthread_create(&threads[i].pid,NULL,A_task,threads[i].thread_body);

		pthread_setschedparam(threads[i].pid ,SCHED_FIFO,&threads[i].thread_body->param);

	}
	pthread_t dispatcher;
	pthread_create(&dispatcher,NULL,dispatch,NULL);
	pthread_barrier_wait(&Barrier);//barrier added so that all threads start at the same time

	struct timespec spec;
	if(runtime>999){//conversion used for execution times greater than 999.999999ms aka 0.999999999s
		if(runtime%1000==0){
			sleep(runtime/1000);
		}
		else{
			sleep(runtime/1000);
			spec.tv_sec = (runtime%1000)/1000;
			spec.tv_nsec = (runtime%1000)*1000000;
			clock_nanosleep(CLOCK_REALTIME,0,&spec,NULL);
		}
	}
	else{
		spec.tv_sec=runtime/1000;
		spec.tv_nsec=runtime*1000000;
		clock_nanosleep(CLOCK_REALTIME,0,&spec,NULL);
	}
	spec.tv_sec = runtime/1000;
	spec.tv_nsec = runtime*1000000;

	clock_nanosleep(CLOCK_REALTIME, 0, &spec, NULL);

	kill(dispatcher,SIGTERM);
	for(int i=0;i<numtasks;i++) kill(threads[i].pid,SIGTERM);


	for(int i=0;i<numtasks;i++){
		args* dummy = threads[i].thread_body;
		while(dummy!=NULL){
			args* dummy2=dummy->next;
			free(dummy);
			dummy=dummy2;	
		}
	}

	die=1;
}


void mutex_init(){//initializes the mutexes
	for(int i=0;i<num_mutexes;i++) 
	{
		pthread_mutex_init(&mutex[i], &mutex_attr[i]);
		pthread_mutexattr_setprotocol(&mutex_attr[i], PTHREAD_PRIO_NONE);
	}
	pthread_mutex_init(&event_mutexes[0], &event_mutexes_attr[0]);
	pthread_mutex_init(&event_mutexes[1], &event_mutexes_attr[1]);

	pthread_mutexattr_setprotocol(&event_mutexes_attr[0], PTHREAD_PRIO_NONE);
	pthread_mutexattr_setprotocol(&event_mutexes_attr[1], PTHREAD_PRIO_NONE);

}

void* dispatch(){	//constantly running, polling for left and right clicks
	int fd;	
	const char *pDevice = MOUSEEVENT;

	struct input_event mouse;

	fd = open(pDevice, O_RDONLY);   
	pthread_barrier_wait(&Barrier);

	while(!die){
		read(fd, &mouse, sizeof(struct input_event));

		if(mouse.code == 0x110) //assume that left and right clicks are mutually exclusive
			pthread_cond_broadcast(&events[0]);//left click

		if(mouse.code == 0x111) 
			pthread_cond_broadcast(&events[1]);// right click
	}
	return NULL;
}

void Compute(int arg){
	int j=0;
	for(int i=0;i<arg;i++) j=j+i;
}

void Lock(int Arg,int priority){
	pthread_mutex_lock(&mutex[Arg]);//locks mutex specified by Arg
}


void Unlock(int Arg){
	pthread_mutex_unlock(&mutex[Arg]);
}

void* P_task(void* Arg){
	args* dummy=(args*)Arg;
	struct timespec spec;	
	long int period = dummy->type_arg;//Main argument for periodic task, the period
	double total_time;

	dummy = (args*) Arg;
	int priority = dummy->param.sched_priority;
	trace_fd = pthread_barrier_wait(&Barrier);


	while(!die)
	{
		clock_gettime(CLOCK_REALTIME,&spec);//grabs start time
		long int start=spec.tv_nsec;
		dummy= (args*) Arg;

		while(dummy!=NULL)
		{
			if(dummy->bodytype==lock) Lock(dummy->body_arg,priority);
			else if(dummy->bodytype==unlock) Unlock(dummy->body_arg);
			else Compute(dummy->body_arg);
			dummy = dummy->next;
		}

		clock_gettime(CLOCK_REALTIME,&spec); //grabs end time and takes the difference between the period and execution
		long int end = spec.tv_nsec;        //and determines whether or not to sleep
		total_time = (double)(end-start) / 1000000.00;

		if( total_time < period)
		{
			spec.tv_sec  =  (period-total_time)/1000;    
			spec.tv_nsec =  (period-total_time)*1000000; 
			clock_nanosleep(CLOCK_REALTIME,0, &spec,NULL);
		}
	}
	return NULL;
}

void* A_task(void* Arg){
	args* dummy=(args*)Arg;
	pthread_barrier_wait(&Barrier);
	int event= dummy->type_arg;//Main argument for aperiodic, 0-left, 1-right
	while(!die)
	{
		dummy=(args*) Arg;
		while(dummy!=NULL)
		{
			if(dummy->bodytype==lock) Lock(dummy->body_arg,1);
			else if(dummy->bodytype==unlock) Unlock(dummy->body_arg);
			else Compute(dummy->body_arg);
			dummy = dummy->next;
		}

		pthread_mutex_lock(&event_mutexes[event]);
		pthread_cond_wait(&events[event], &event_mutexes[event]);//pthread_cond used for block aperiodic tasks for mouse events
		pthread_mutex_unlock(&event_mutexes[event]);
	}
	return NULL;
}
