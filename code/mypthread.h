// File:	mypthread_t.h

// List all group member's name: Michael Yen (mjy37), Jay Patel (jsp202)
// username of iLab: ilab1
// iLab Server: ilab1.cs.rutgers.edu



#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

#define LOWEST_PRIORITY 3 // LOWEST_PRIORITY + 1 = TOTAL NUMBER OF QUEUE LEVELS FOR MLFQ
#define STACK_SIZE 32000 // 32 kb
#define TIME_QUANTUM 15 //milliseconds, for STCF
#define TIME_PERIOD_S 45 //milliseconds, for MLFQ
#ifdef MLFQ
	#define SCHED MLFQ_SCHEDULER
#elif FIFO
  #define SCHED FIFO_SCHEDULER
#else
  #define SCHED STCF_SCHEDULER
#endif

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ucontext.h>
#include <malloc.h>
#include <signal.h>



typedef uint mypthread_t;

typedef enum _status{
READY,RUNNING,BLOCKED
}status;

typedef enum _scheduler{
MLFQ_SCHEDULER, FIFO_SCHEDULER, STCF_SCHEDULER
}scheduler;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	struct mypthread_mutex_t* blocked_by;
	struct my_queue_node* next_blocked;
	struct my_queue_node* next_join_blocked;
	// thread Id
	mypthread_t Id;

	// thread status
	status Status;

	// thread context
	ucontext_t * Context;
	//ucontext_t RetContext;

	// thread stack
	// thread priority
	int Priority;
	// And more ...
	int TimeQuantums; // used for stcf
	void * return_value;
	int time_passed; //in microseconds, used for mlfq

	// YOUR CODE HERE
} threadControlBlock;

typedef struct my_mutex_node{
	struct my_mutex_node* next;
	struct mypthread_mutex_t* mutex;
} my_mutex_node;

typedef struct my_queue_node{
	struct threadControlBlock* t_tcb;
	struct my_queue_node* next;
} my_queue_node;

typedef struct my_queue{
	struct my_queue_node* first;
	struct my_queue_node* last;
	int timeslice;
} my_queue;

typedef struct my_multi_queue{
	my_queue ** queue_arr;
} my_multi_queue;

void* retval[1000000];

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */
	int mId;
	int isLocked;
	my_queue_node* node_has_lock;
	my_queue_node* node_blocked_list;

	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
