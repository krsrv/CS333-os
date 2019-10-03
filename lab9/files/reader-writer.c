#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

//#define DEBUG

struct read_write_lock
{
	pthread_mutex_t lock;	// If required, use this strucure to create
	int reader_count;
	pthread_mutex_t writer_lock;

	int writer_incoming;
	pthread_cond_t writer_in_queue;
	pthread_mutex_t pref_lock;
}rwlock;

struct argument_t
{
	int id;
	int delay;
};

long int data = 0;			//	Shared data variable

void InitalizeReadWriteLock(struct read_write_lock * rw)
{
	pthread_mutex_init(&rw->lock, NULL);
	rw->reader_count = 0;
	pthread_mutex_init(&rw->writer_lock, NULL);
	
	pthread_cond_init(&rw->writer_in_queue, NULL);
	pthread_mutex_init(&rw->pref_lock, NULL);
	rw->writer_incoming = 0;
}


// The pthread based reader lock
void ReaderLock(struct read_write_lock * rw)
{
	pthread_mutex_lock(&rw->pref_lock);
	while(rw->writer_incoming > 0)
		  pthread_cond_wait(&rw->writer_in_queue, &rw->pref_lock);
	pthread_mutex_unlock(&rw->pref_lock);

	pthread_mutex_lock(&rw->lock);

	if(rw->reader_count == 0)
		pthread_mutex_lock(&rw->writer_lock);
	rw->reader_count++;

	pthread_mutex_unlock(&rw->lock);
}	

// The pthread based reader unlock
void ReaderUnlock(struct read_write_lock * rw)
{
	pthread_mutex_lock(&rw->lock);
	rw->reader_count--;
	if(rw->reader_count == 0){
		pthread_mutex_unlock(&rw->writer_lock);
	}
	pthread_mutex_unlock(&rw->lock);
}

// The pthread based writer lock
void WriterLock(struct read_write_lock * rw)
{
	pthread_mutex_lock(&rw->pref_lock);
	rw->writer_incoming++;
	pthread_mutex_unlock(&rw->pref_lock);

	pthread_mutex_lock(&rw->writer_lock);
}

// The pthread based writer unlock
void WriterUnlock(struct read_write_lock * rw)
{
	pthread_mutex_unlock(&rw->writer_lock);

	pthread_mutex_lock(&rw->pref_lock);
	rw->writer_incoming--;
	pthread_mutex_unlock(&rw->pref_lock);
	if(rw->writer_incoming == 0)
		pthread_cond_broadcast(&rw->writer_in_queue);
}

//	Call this function to delay the execution by 'delay' ms
void delay(int delay)
{
	struct timespec wait;

	if(delay <= 0)
		return;

	wait.tv_sec = delay / 1000;
	wait.tv_nsec = (delay % 1000) * 1000 * 1000;
	nanosleep(&wait, NULL);
}

// The pthread reader start function
void *ReaderFunction(void *args)
{
	struct argument_t *arg = (struct argument_t *) args;
	int del = arg->delay, id = arg->id;
	delay(del);

#ifdef DEBUG
	printf("Trying lock for Reader %d\n", id);
#endif
	ReaderLock(&rwlock);
	
#ifndef DEBUG
	printf("Reader\t%d, data:\t%ld\n", id, data);
#endif

	delay(1);
#ifdef DEBUG
	printf("Releasing lock for Reader %d\n", id);
#endif
	ReaderUnlock(&rwlock);
	//	Delay the execution by arrival time specified in the input
	
	//	....
	
	//  Get appropriate lock
	//	Display  thread ID and value of the shared data variable
	//
    //  Add a dummy delay of 1 ms before lock release  
	//	....
}

// The pthread writer start function
void *WriterFunction(void *args)
{
	struct argument_t *arg = (struct argument_t *) args;
	int del = arg->delay, id = arg->id;
	delay(del);
#ifdef DEBUG
	printf("\tTrying lock for Writer %d\n", id);
#endif
	WriterLock(&rwlock);
	
	data++;
#ifndef DEBUG
	printf("\tWriter\t%d, data:\t%ld\n", id, data);
#endif

	delay(1);
#ifdef DEBUG
	printf("\tReleasing lock for Writer %d\n", id);
#endif
	WriterUnlock(&rwlock);
	//	Delay the execution by arrival time specified in the input
	
	//	....
	//
	//  Get appropriate lock
	//	Increment the shared data variable
	//	Display  thread ID and value of the shared data variable
	//
    //  Add a dummy delay of 1 ms before lock release  
	//	....
	
}

int main(int argc, char *argv[])
{
	pthread_t *threads;
	struct argument_t *arg;
	
	long int reader_number = 0;
	long int writer_number = 0;
	long int thread_number = 0;
	long int total_threads = 0;	
	
	int count = 0;			// Number of 3 tuples in the inputs.	Assume maximum 10 tuples 
	int rws[10];			
	int nthread[10];
	int delay[10];

	//	Verifying number of arguments
	if(argc<4 || (argc-1)%3!=0)
	{
		printf("reader-writer <r/w> <no-of-threads> <thread-arrival-delay in ms> ...\n");		
		printf("Any number of readers/writers can be added with repetitions of above mentioned 3 tuple \n");
		exit(1);
	}

	//	Reading inputs
	for(int i=0; i<(argc-1)/3; i++)
	{
		char rw[2];
		strcpy(rw, argv[(i*3)+1]);

		if(strcmp(rw, "r") == 0 || strcmp(rw, "w") == 0)
		{
			if(strcmp(rw, "r") == 0)
				rws[i] = 1;					// rws[i] = 1, for reader
			else
				rws[i] = 2;					// rws[i] = 2, for writer
			
			nthread[i] = atol(argv[(i*3)+2]);		
			delay[i] = atol(argv[(i*3)+3]);

			count ++;						//	Number of tuples
			total_threads += nthread[i];	//	Total number of threads
		}
		else
		{
			printf("reader-writer <r/w> <no-of-threads> <thread-arrival-delay in ms> ...\n");
			printf("Any number of readers/writers can be added with repetitions of above mentioned 3 tuple \n");
			exit(1);
		}
	}

	threads = (pthread_t *)malloc(total_threads * sizeof(pthread_t));
	arg = (struct argument_t *)malloc(total_threads * sizeof(struct argument_t));
	//	Create reader/writer threads based on the input and read and write.
	for(int i=0; i<(argc-1)/3; i++) 
	{
		for(int j = 0; j < nthread[i]; j++) {
			if(rws[i] == 1) {
				arg[thread_number].id = reader_number+1;
				arg[thread_number].delay = delay[i];
				pthread_create(&threads[thread_number], NULL, ReaderFunction, (void *)&arg[thread_number]);
				reader_number++;
			} else {
				arg[thread_number].id = writer_number+1;
				arg[thread_number].delay = delay[i];
				pthread_create(&threads[thread_number], NULL, WriterFunction, (void *)&arg[thread_number]);
				writer_number++;
			}
			thread_number++;
		}
	}

	for(int i = 0; i < total_threads; i++)
		pthread_join(threads[i], NULL);

	//  Clean up threads on exit

	exit(0);
}
