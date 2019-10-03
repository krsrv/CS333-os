#include <stdio.h>
#include <pthread.h>

#define N_THREADS 10

pthread_mutex_t mutex[N_THREADS];
pthread_t thread[N_THREADS];
int counter[N_THREADS];
int argument[N_THREADS];

void * count(void *arg)
{
	int index = *(int *)arg, i;
	for(i = 0; i < 1000; i++) {
		pthread_mutex_lock(&mutex[index]);
		counter[index] += 1;
		pthread_mutex_unlock(&mutex[index]);
	}
	
	return NULL;
}

int main()
{
	int i, j;

	for(i = 0; i < N_THREADS; i++) {
		pthread_mutex_init(&mutex[i], NULL);
		counter[i] = 0;
		argument[i] = i;
	}

	for(i = 0; i < N_THREADS; i++) {
		pthread_create(&thread[i], NULL, &count, (void *)&argument[i]);
	}

	for(j = 0; j < 1000; j++) for(i = 0; i < N_THREADS; i++) {
		pthread_mutex_lock(&mutex[i]);
		counter[i] += 1;
		pthread_mutex_unlock(&mutex[i]);
	}

	for(i = 0; i < N_THREADS; i++)
		pthread_join(thread[i], NULL);
	
	for(i = 0; i < N_THREADS; i++)
		printf("Counter[%d]: %d\n", i, counter[i]);

	return 0;
}

