// File:	mypthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "mypthread.h"



// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
int ignore_int = 0;
int num_thread = 0;
int num_mutex = 0;
int yielded=0;
ucontext_t schedulerContext;
ucontext_t parentContext;
queue* threadqueue = NULL;
multi_queue* multiqueue = NULL;
queue_node* runningnode = NULL;
mutex_node* mutexlist = NULL;
mypthread_mutex_t queuelock;

/* Extra function declarations */
queue_node* find_node(mypthread_t thread);
void enqueue(queue* queue, queue_node* queue_node);
queue_node* dequeue(queue* queue);
void processFinishedJob(int tID);
void free_queue_node(queue_node* runningnode);

/* Extra function declarations end */

/*
void threadWrapper(void* arg, void*(*function)(void*), int tID){
	void* threadretval = (*function)(arg);
	retval[tID] = threadretval;
	
}
*/

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

		ignore_int  = 1;
       // create Thread Control Block
	   *thread = ++num_thread;

		//create schedular context on an uninitialized queue
		if(threadqueue == NULL){
			getcontext(&schedulerContext);
			schedulerContext.uc_link = 0;
			schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);
			schedulerContext.uc_stack.ss_size = STACK_SIZE;

			if(schedulerContext.uc_stack.ss_sp == 0){
				printf("Couldn't allocate space for schedular context\n");
				exte(1);
			}

			schedulerContext.uc_stack.ss_flags = 0;
			makecontext(&schedulerContext, (void*)&schedule, 0);

		}



		tcb* new_tcb = (tcb*) malloc(sizeof(tcb));
		new_tcb->Priority = 0;
		new_tcb->TimeRan = 0;
		new_tcb->Id = *thread;
		new_tcb->Status = READY;
		new_tcb->blocked_by = NULL;
		new_tcb->next_blocked = NULL;


    	// create and initialize the context of this thread
    	// allocate space of stack for this thread to run
		/*
		getcontext(&(new_tcb->RetContext));
		new_tcb->RetContext.uc_link = &schedulerContext;
		new_tcb->RetContext.uc_stack.ss_sp = malloc(STACK_SIZE);

		if(new_tcb->RetContext.uc_stack.ss_size == 0){
			printf("Couldn't allocate space for return context\n");
			exit(1);
		}

		new_tcb->RetContext.uc_stack.ss_size = STACK_SIZE;
		new_tcb->RetContext.uc_stack.ss_flags = 0;

		makecontext(&new_tcb->RetContext, (void*)&processFinishedJob, 1, new_tcb->Id);

		printf("new thread has been created: %d\n", *thread);
		*/
		// after everything is all set, push this thread int
		// YOUR CODE HERE

		ucontext_t newThreadContext;
		getcontext(&newThreadContext);

		newThreadContext.uc_link = &schedulerContext;
		newThreadContext.uc_stack.ss_sp = malloc(STACK_SIZE);

		if(newThreadContext.uc_stack.ss_sp == 0){
		   printf("Couldn't allocate space for context\n");
		   exit(0);
		}

		newThreadContext.uc_stack.ss_size = STACK_SIZE;
		newThreadContext.uc_stack.ss_flags = 0;

		//makecontext(&newThreadContext, (void*)threadWrapper, 3, arg, function, (int)&new_tcb->Id);
		makecontext(&newThreadContext, (void*)function, 1, arg);
		new_tcb->Context = newThreadContext;

		queue_node* qnode = (queue_node*) malloc(sizeof(queue_node));
		qnode->t_tcb = new_tcb;
		qnode->next = NULL;


		//put thread in a queue


		if(threadqueue == NULL){ //initialize queue/multi queue depending on the scheduler

			if(SCHED == FIFO_SCHEDULER){
				threadqueue = (queue*) malloc(sizeof(queue));
				threadqueue->first = qnode;
				threadqueue->last = qnode;
			}
			else if(SCHED == PSJF_SCHEDULER){
				threadqueue = (queue*) malloc(sizeof(queue));
				threadqueue->first = qnode;
				threadqueue->last = qnode;
			}
			else{ //MLFQ
				multiqueue = (multi_queue*) malloc(sizeof(multi_queue));
				multiqueue->queue0 = (queue*) malloc(sizeof(queue));
				multiqueue->queue1 = (queue*) malloc(sizeof(queue));
				multiqueue->queue2 = (queue*) malloc(sizeof(queue));
				multiqueue->queue3 = (queue*) malloc(sizeof(queue));

				multiqueue->queue0->first = qnode;
				multiqueue->queue0->last = qnode;
				multiqueue->queue1->first = NULL;
				multiqueue->queue1->last = NULL;
				multiqueue->queue2->first = NULL;
				multiqueue->queue2->last = NULL;
				multiqueue->queue3->first = NULL;
				multiqueue->queue3->last = NULL;
			}

			mypthread_mutex_init(&queuelock, NULL);
			getcontext(&parentContext);

		}
		else{ //thread is already initialized

			if(SCHED == FIFO_SCHEDULER){
				if(threadqueue->first == NULL){ //if thread is null
					threadqueue->first = qnode;
					threadqueue->last = qnode;
				}
				else{
					queue_node* last_qnode = threadqueue->last;
					last_qnode->next = qnode;
					threadqueue->last = qnode;
				}
			}
			else if(SCHED == PSJF_SCHEDULER){
				if(threadqueue->first == NULL){
					threadqueue->first = qnode;
					threadqueue->last = qnode;
				}
				else{
					queue_node* last_node = threadqueue->last;
					last_node->next = qnode;
					threadqueue->last = qnode;
				}
			}
			else{ //MLFQ

				if(multiqueue->queue0->first == NULL){
					multiqueue->queue0->first = qnode;
					multiqueue->queue0->last = qnode;
				}
				else{
					queue_node* last_node = multiqueue->queue0->last;
					last_node->next = qnode;
					multiqueue->queue0->last = qnode;
				}
			}
		}

		ignore_int = 0;
		return 0;

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	yielded++;
	runningnode->t_tcb->Status = READY;
	swapcontext(&(runningnode->t_tcb->Context), &schedulerContext); // save to return context and switch to scheduler context
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	runningnode->t_tcb->Status = EXIT;
	if(value_ptr != NULL){
		value_ptr = runningnode->t_tcb->return_value; // if value_ptr not NULL, save return value from thread
	}
	free_queue_node(runningnode); // deallocate memory
	setcontext(&schedulerContext); // go to scheduler
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	queue_node* node = find_node(thread); // find blocking node
	while(node->t_tcb->Status != EXIT){ // need the tcb status
		mypthread_yield(); // do not run until it is done
	}
	if(value_ptr != NULL){ 
		value_ptr = node->t_tcb->return_value; // if value_ptr not NULL, save return value from thread
	}
	free_queue_node(node);
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE

	mutex_node* mutexnode = (mutex_node*) malloc(sizeof(mutex_node));
	mypthread_mutex_t* newmutex = (mypthread_mutex_t*) malloc(sizeof(mypthread_mutex_t));
	mutexnode->mutex = newmutex;
	mutexnode->mutex->isLocked = 0;
	mutexnode->mutex->mId = ++num_mutex;
	mutexnode->next = mutexlist;
	mutexlist = mutexnode;
	*mutex = *newmutex;

	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {

	ignore_int = 1;
	mutex_node* mutextolock = find_mutex(mutex->mId);

	if(mutextolock == NULL){
		ignore_int = 0;
		return -1;
	}

    // use the built-in test-and-set atomic function to test the mutex

	if(__sync_lock_test_and_set(&(mutextolock->mutex->isLocked), 1) == 0){
    // if the mutex is acquired successfully, enter the critical section
		mutextolock->mutex->node_has_lock = runningnode;
		mutextolock->mutex->node_blocked_list = NULL;
		ignore_int = 0;
		return 0;
	}
    // if acquiring mutex fails, push current thread into block list and //
    // context switch to the scheduler thread

	if(mutextolock->mutex->node_blocked_list == NULL){
		mutextolock->mutex->node_blocked_list = runningnode;
	}
	else{
		mutextolock->mutex->node_blocked_list->t_tcb->next_blocked = runningnode;
	}
	runningnode->t_tcb->Status = BLOCKED;
	setcontext(&schedulerContext);

	//mypthread_yield();

    // YOUR CODE HERE
	ignore_int = 0;
    return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {

	ignore_int = 1;
	mutex_node* mutextounlock = find_mutex(mutex->mId);

	if(mutextounlock == NULL){
		ignore_int = 0;
		return -1;
	}

	// Release mutex and make it available again.
	mutextounlock->mutex->isLocked = 0;
	mutextounlock->mutex->node_has_lock = NULL;

	// Put threads in block list to run queue
	// so that they could compete for mutex later.


	// YOUR CODE HERE
	ignore_int = 0;
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init
	ignore_int = 1;
	mutex_node* mutextodestroy = find_mutex(mutex->mId);

	if(mutextodestroy == NULL){
		ignore_int = 0;
		return -1;
	}

	if(mutextodestroy->mutex->isLocked == 1){
		mutextodestroy->mutex->isLocked = 0;
		mutextodestroy->mutex->node_has_lock = NULL;
		mutextodestroy->mutex->node_blocked_list = NULL;
	}

	mutex_node* prevmutex = find_prev_mutex(mutex->mId);
	if(mutextodestroy->next != NULL){
		if(prevmutex != NULL){
			prevmutex->next = mutextodestroy->next;
		}
	}
	else{
		if(prevmutex != NULL){
			prevmutex->next = NULL;
		}
	}

	free(mutextodestroy->next);
	free_queue_node(mutextodestroy->mutex->node_has_lock);
	free_queue_node(mutextodestroy->mutex->node_blocked_list);
	free(mutextodestroy);

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

void free_queue_node(queue_node* runningnode){
	if(runningnode == NULL){
		return;;
	}
	free(runningnode->t_tcb->Context.uc_stack.ss_sp); // deallocate all memory for queue node
	//free(runningnode->t_tcb->RetContext.uc_stack.ss_sp);
	free(runningnode->t_tcb);
	free(runningnode);		
}

queue_node* find_node(mypthread_t thread){ // look through queue to find the queue node corresponding to a thread

}

void enqueue(queue* queue, queue_node* queue_node){

}

queue_node* dequeue(queue* queue){

}

void processFinishedJob(int tID){

}

mutex_node* find_mutex(int mutexid){
	ignore_int = 1;

	mutex_node* mptr = mutexlist;
	while(mptr != NULL){
		if(mptr->mutex->mId == mutexid){
			ignore_int = 0;
			return mptr;
		}
		else{
			mptr = mptr->next;
		}
	}
	ignore_int = 0;
	return NULL;
}

mutex_node* find_prev_mutex(int mutexid){
	ignore_int = 1;

	mutex_node* mptr = mutexlist;
	while(mptr->next != NULL){
		if(mptr->next->mutex->mId == mutexid){
			ignore_int = 0;
			return mptr;
		}
		else{
			mptr = mptr->next;
		}
	
	}
	ignore_int = 0;
	return NULL;
}



