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

void thread1(){
	printf("This is thread1\n");
	mypthread_exit(NULL);
	printf("still here\n");
}

void thread2(){
	printf("This is thread2");
}

int main(int argc, char **argv) {

	mypthread_t t1;
	// mypthread_t t2;

	mypthread_create(&t1, NULL, (void *)&thread1, NULL);
	// mypthread_create(&t2, NULL, (void *)&thread1, NULL);
	printf("Still here\n");
	/* Implement HERE */
	mypthread_join(t1, NULL);
	printf("Main Done\n");
	return 0;
}
