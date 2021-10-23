#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
pthread_mutex_t mutex;
long long int sum = 0;

void thread1(){
	for(long long int i = 0; i < 1000000000; i++){
		pthread_mutex_lock(&mutex);
		sum += 1;
		pthread_mutex_unlock(&mutex);
	}

	mypthread_exit(NULL);
}

void thread2(){
	for(long long int i = 0; i < 2000000000; i++){
		pthread_mutex_lock(&mutex);
		sum += 1;
		pthread_mutex_unlock(&mutex);
	}
	mypthread_exit(NULL);
}

void thread3(){
	for(long long int i = 0; i < 3000000000; i++){
		pthread_mutex_lock(&mutex);
		sum += 1;
		pthread_mutex_unlock(&mutex);
	}
	mypthread_exit(NULL);
}

int main(int argc, char **argv) {

	mypthread_t t1;
	mypthread_t t2;
	mypthread_t t3;

	mypthread_create(&t1, NULL, (void *)&thread1, NULL);
	mypthread_create(&t2, NULL, (void *)&thread2, NULL);
	wait(20);
	for (long long int i = 0; i < 2000000000; i++);
	mypthread_create(&t3, NULL, (void *)&thread3, NULL);

	// pthread_mutex_init(&mutex, NULL);
	// mypthread_create(&t2, NULL, (void *)&thread1, NULL);
	/* Implement HERE */
	
	mypthread_join(t1, NULL);
	printf("joined t1\n");
	mypthread_join(t2, NULL);
	printf("joined t2\n");
	mypthread_join(t3, NULL);
	printf("Main Done\n");

	printf("sum is: %lld\n", sum);
	// pthread_mutex_destroy(&mutex);

	return 0;
}
