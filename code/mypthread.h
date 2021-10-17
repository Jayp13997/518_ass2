// File:	mypthread_t.h

// List all group member's name:
// username of iLab:
// iLab Server:



#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

#define LOWEST_PRIORITY 3
#define STACK_SIZE 1048576//A megabyte
#define TIME_QUANTUM 15//milliseconds
#ifdef MLFQ
	#define SCHED MLFQ_SCHEDULER
#elif FIFO
  #define SCHED FIFO_SCHEDULER
#else
  #define SCHED PSJF_SCHEDULER
#endif

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>//added
#include <sys/time.h>
#include <ucontext.h>
#include <malloc.h>



typedef uint mypthread_t;

 typedef enum _status{
   READY,RUNNING,BLOCKED,EXIT,WAIT
 }status;

 typedef enum _scheduler{
   MLFQ_SCHEDULER, FIFO_SCHEDULER, PSJF_SCHEDULER
 }scheduler;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	struct mypthread_mutex_t* blocked_by;
	// thread Id
	mypthread_t* Id;

	// thread status
	status Status;

	// thread context
	ucontext_t Context;
	ucontext_t RetContext; // don't think we need it

	// thread stack
	// thread priority
	int Priority;
	// And more ...
	unsigned long int TimeRan;
	void * return_value;

	// YOUR CODE HERE
} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */
	int mId;
	int isLocked;
	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

typedef struct my_mutex_node{
	struct my_mutex_node* next;
	mypthread_mutex_t* mutex;
} mutex_node;

typedef struct my_queue_node{
	tcb* t_tcb;
	struct my_queue_node* next;
} queue_node;

typedef struct my_queue{
	struct queue_node* first;
	struct queue_node* last;
} queue;

typedef struct my_multi_queue{
	queue* queue0;
	queue* queue1;
	queue* queue2;
	queue* queue3;
} multi_queue;


void* retval[1000000];


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
