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
ucontext_t mainContext;
// ucontext_t parentContext;
my_queue* threadqueue = NULL;
my_multi_queue* multiqueue = NULL;
my_queue_node* runningnode = NULL;
my_mutex_node* mutexlist = NULL;
mypthread_mutex_t queuelock;
struct itimerval timer;
struct sigaction myAction;

/* Extra function declarations */
my_queue_node* find_node(mypthread_t thread);
void enqueue(my_queue* queue, my_queue_node* queue_node);
my_queue_node* dequeue(my_queue* queue);
void processFinishedJob(int tID);
void free_queue_node(my_queue_node* runningnode);
static void schedule();
my_mutex_node* find_mutex(int mutexid);
my_mutex_node* find_prev_mutex(int mutexid);
static void sched_fifo();
void print_queue(my_queue* queue_print);
my_queue_node* psjf_dequeue(my_queue* queue);
my_queue_node* get_prev_node(my_queue* queue, my_queue_node* node);

/* Extra function declarations end */

/*
void threadWrapper(void* arg, void*(*function)(void*), int tID){
	void* threadretval = (*function)(arg);
	retval[tID] = threadretval;
	
}
*/

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	printf("Mypthread create was called.\n");

	ignore_int = 1;
	num_thread ++;
	// create Thread Control Block
	*thread = num_thread;

	//create schedular context on an uninitialized queue
	if(threadqueue == NULL){
		//getcontext(&mainContext);
		getcontext(&schedulerContext);
		schedulerContext.uc_link = 0;
		schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		schedulerContext.uc_stack.ss_size = STACK_SIZE;

		if(schedulerContext.uc_stack.ss_sp == 0){
			printf("Couldn't allocate space for schedular context\n");
			exit(1);
		}

		schedulerContext.uc_stack.ss_flags = 0;
		makecontext(&schedulerContext, (void*)&schedule, 0);

	}

	threadControlBlock* new_tcb = (threadControlBlock*) malloc(sizeof(threadControlBlock));
	new_tcb->Priority = 0;
	new_tcb->TimeQuantums = 0;
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
	printf("TCB Context Created\n");

	my_queue_node* qnode = (my_queue_node*) malloc(sizeof(my_queue_node));
	qnode->t_tcb = new_tcb;
	qnode->next = NULL;


	//put thread in a queue
	printf("This is node: %d\n", new_tcb->Id);
	print_queue(threadqueue);
	if(threadqueue == NULL){ //initialize queue/multi queue depending on the scheduler //start timer
		printf("Creates queue\n");

		if(SCHED == FIFO_SCHEDULER){
			printf("Using FIFO\n");
			threadqueue = (my_queue*) malloc(sizeof(my_queue));
			threadqueue->first = qnode;
			threadqueue->last = qnode;
			threadqueue->timeslice = NULL;
		}
		else if(SCHED == PSJF_SCHEDULER){
			printf("Using PSJF\n");
			threadqueue = (my_queue*) malloc(sizeof(my_queue));
			threadqueue->first = qnode;
			threadqueue->last = qnode;
			threadqueue->timeslice = NULL;
			printf("Created queue\n");
		}
		else{ //MLFQ
			printf("Using MLFQ\n");
			multiqueue = (my_multi_queue*) malloc(sizeof(my_multi_queue));
			// multiqueue->queue0 = (my_queue*) malloc(sizeof(my_queue));
			// multiqueue->queue1 = (my_queue*) malloc(sizeof(my_queue));
			// multiqueue->queue2 = (my_queue*) malloc(sizeof(my_queue));
			// multiqueue->queue3 = (my_queue*) malloc(sizeof(my_queue));

			// multiqueue->queue0->first = qnode;
			// multiqueue->queue0->last = qnode;
			// multiqueue->queue1->first = NULL;
			// multiqueue->queue1->last = NULL;
			// multiqueue->queue2->first = NULL;
			// multiqueue->queue2->last = NULL;
			// multiqueue->queue3->first = NULL;
			// multiqueue->queue3->last = NULL;
			multiqueue->queue_arr = (my_queue**) malloc(MULTIQUEUE_NUM * sizeof(my_queue*));
			multiqueue->queue_arr[0]->first = qnode;
			multiqueue->queue_arr[0]->last = qnode;
			for(int i = 1; i <= MULTIQUEUE_NUM; i++){
				multiqueue->queue_arr[i]->first = NULL;
				multiqueue->queue_arr[i]->last = NULL;
			}
		}
		//start_timer();
		//mypthread_mutex_init(&queuelock, NULL);
		print_queue(threadqueue);
		// swapcontext(&mainContext, &schedulerContext);
		// getcontext(&runningnode->t_tcb->Context);
		getcontext(&mainContext);
	}
	else{ //thread is already initialized

		if(SCHED == FIFO_SCHEDULER){
			if(threadqueue->first == NULL){ //if thread is null
				threadqueue->first = qnode;
				threadqueue->last = qnode;
			}
			else{
				my_queue_node* last_qnode = threadqueue->last;
				last_qnode->next = qnode;
				threadqueue->last = qnode;
			}
			//call scheduler
		}
		else if(SCHED == PSJF_SCHEDULER){
			if(threadqueue->first == NULL){
				threadqueue->first = qnode;
				threadqueue->last = qnode;
			}
			else{
				printf("adding another node to queue\n");
				my_queue_node* last_node = threadqueue->last;
				last_node->next = qnode;
				threadqueue->last = qnode;
				printf("The first node is id: %d Status %d \n", threadqueue->first->t_tcb->Id,threadqueue->first->t_tcb->Status);
				printf("The second node is id: %d Status %d \n", threadqueue->last->t_tcb->Id,threadqueue->last->t_tcb->Status);
			}
			
		}
		else{ //MLFQ

			// if(multiqueue->queue0->first == NULL){
			// 	multiqueue->queue0->first = qnode;
			// 	multiqueue->queue0->last = qnode;
			// }
			// else{
			// 	my_queue_node* last_node = multiqueue->queue0->last;
			// 	last_node->next = qnode;
			// 	multiqueue->queue0->last = qnode;
			// }
			if(multiqueue->queue_arr[0]->first == NULL){
				multiqueue->queue_arr[0]->first = qnode;
				multiqueue->queue_arr[0]->last = qnode;
			}
			else{
				my_queue_node* last_node = multiqueue->queue_arr[0]->last;
				last_node->next = qnode;
				multiqueue->queue_arr[0]->last = qnode;
			}
			//call scheduler
		}
		setcontext(&schedulerContext);
		getcontext(&runningnode->t_tcb->Context);
	}
	print_queue(threadqueue);
	ignore_int = 0;
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
	swapcontext((&runningnode->t_tcb->Context), &schedulerContext); // save to return context and switch to scheduler context
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	if(value_ptr != NULL){
		value_ptr = &runningnode->t_tcb->return_value; // if value_ptr not NULL, save return value from thread
	}
	printf("try freeing node\n");
	my_queue_node * ptr = runningnode;
	runningnode = NULL;
	//free_queue_node(ptr); // deallocate memory - need to eventually figure this out
	printf("freed node\n");
	setcontext(&schedulerContext); // go to scheduler
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	my_queue_node* node = find_node(thread); // find blocking node
	while(node != NULL){ // while the thread is not terminated
		mypthread_yield(); // do not run until it is done
	}
	if(value_ptr != NULL){ 
		value_ptr = &node->t_tcb->return_value; // if value_ptr not NULL, save return value from thread
	}
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE

	my_mutex_node* mutexnode = (my_mutex_node*) malloc(sizeof(my_mutex_node));
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
	my_mutex_node* mutextolock = find_mutex(mutex->mId);

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
	my_mutex_node* mutextounlock = find_mutex(mutex->mId);

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
	my_mutex_node* mutextodestroy = find_mutex(mutex->mId);

	if(mutextodestroy == NULL){
		ignore_int = 0;
		return -1;
	}

	if(mutextodestroy->mutex->isLocked == 1){
		mutextodestroy->mutex->isLocked = 0;
		mutextodestroy->mutex->node_has_lock = NULL;
		mutextodestroy->mutex->node_blocked_list = NULL;
	}

	my_mutex_node* prevmutex = find_prev_mutex(mutex->mId);
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
	/*
	Ideas: Schedule() is called whenever a thread finishes, yields, is blocked, or interrupted
	Need to implement a timer interrupt
	Scheduler should also book keep the time something ran so it can put into tcb block
	Use int setitmer() and int sigaction()
	*/
	printf("scheduler being called\n");
	if(isEmpty(threadqueue)){
		setcontext(&mainContext);
	}
// schedule policy
#ifndef MLFQ
	// Choose STCF
// #elif FIFO
// 	sched_fifo();
// 	// Choose MLFQ
	sched_fifo();
#else
	sched_fifo();
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)
	printf("PSJF called\n");
	
	if(runningnode == NULL){
		runningnode = psjf_dequeue(threadqueue);

		printf("dequeued node\n");
		printf("runnning node is: %d\n", runningnode->t_tcb->Id);
		//cannot run a blocked thread
		while(runningnode->t_tcb->Status == BLOCKED){
			printf("status is blocked\n");
			enqueue(threadqueue, runningnode);
			runningnode = psjf_dequeue(threadqueue);
		}
		runningnode->t_tcb->Status = RUNNING;
		printf("Set status to running\n");
		print_queue(threadqueue);
		setcontext(&(runningnode->t_tcb->Context));
		printf("set context failed\n");
	}
	//Thread Yielded, runningnode is READY
	//Thread Blocked, runningnode is BLOCKED
	//Thread Interrupted (timer), runningnode is RUNNING
	else {
		if (runningnode->t_tcb == RUNNING){ // was interrupted
			runningnode->t_tcb = READY;
		}
		enqueue(threadqueue, runningnode);

		// cannot run a blocked thread
		runningnode = psjf_dequeue(threadqueue);
		while(runningnode->t_tcb->Status == BLOCKED){
			enqueue(threadqueue, runningnode);
			runningnode = dequeue(threadqueue);
		}
		runningnode->t_tcb->Status = RUNNING;
		setcontext(&(runningnode->t_tcb->Context));
	}

	// YOUR CODE HERE
	//Thread Finished, runningnode is NULL - schedule new runningnode

	//Thread Yielded, runningnode is READY
	//Thread Blocked, runningnode is BLOCKED
	//Thread Interrupted (timer), runningnode is RUNNING
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	// Rule 1: If Priority(A) > Priority(B), Run A and not B
	// Rule 2: If Priority(A) = Priority(B), Run A and B in Round Robin
	// Rule 3: When a job enters the system, it is placed at the highest priority
	// Rule 4: Once a job uses up its time allotment at a given level, its priority is reduced
	// Rule 5: After some time period S, move all jobs in the system to the topmost queue.

	//Thread Finished, runningnode is NULL - schedule new runningnode starting from queues of most importance
	if(runningnode == NULL){
		int i = 0;
		while(isEmpty(multiqueue) && allBlocked(multiqueue) && i <= 3){ // get multiqueue queue that is not empty and not all blocked
			i++;
		}
		if(i > 3){ // no more threads to run
			//handle
		}
		runningnode = dequeue(multiqueue->queue_arr[i]);
		printf("dequeued node\n");
		printf("runnning node is: %d\n", runningnode->t_tcb->Id);

		while(runningnode->t_tcb->Status == BLOCKED){
			printf("status is blocked\n");
			enqueue(multiqueue->queue_arr[i], runningnode);
			runningnode = dequeue(multiqueue->queue_arr[i]);
		}
		runningnode->t_tcb->Status = RUNNING;
		printf("Set status to running\n");
		print_queue(multiqueue->queue_arr[i]);
		setcontext(&(runningnode->t_tcb->Context));
		printf("set context failed\n");
	}
	//Thread Yielded, runningnode is READY
	//Thread Blocked, runningnode is BLOCKED
	//Thread Interrupted (timer), runningnode is RUNNING
	else {
		//Thread Interrupted - increment 
		runningnode->t_tcb->TimeQuantums ++;
		if(runningnode->t_tcb->Priority < 3 && runningnode->t_tcb->TimeQuantums > multiqueue->queue_arr[runningnode->t_tcb->Priority]->timeslice){ //Exceeded time period, increment priority / decrease importance
			runningnode->t_tcb->Priority ++;
			enqueue(multiqueue->queue_arr[runningnode->t_tcb->Priority], runningnode);
			runningnode = NULL;
		}
		else { // has not exceeded time period, round robin
			enqueue(multiqueue->queue_arr[runningnode->t_tcb->Priority], runningnode);
			runningnode = dequeue(threadqueue);
			while(runningnode->t_tcb->Status == BLOCKED){
				enqueue(threadqueue, runningnode);
				runningnode = dequeue(threadqueue);
			}
		}
	}
}

// Feel free to add any other functions you need

// YOUR CODE HERE
static void sched_fifo() {
	//Thread Finished, runningnode is NULL - schedule new runningnode
	printf("FIFO called\n");
	print_queue(threadqueue);
	if(runningnode == NULL){
		runningnode = dequeue(threadqueue);
		printf("dequeued node\n");
		printf("runnning node is: %d\n", runningnode->t_tcb->Id);
		//cannot run a blocked thread
		while(runningnode->t_tcb->Status == BLOCKED){
			printf("status is blocked\n");
			enqueue(threadqueue, runningnode);
			runningnode = dequeue(threadqueue);
		}
		runningnode->t_tcb->Status = RUNNING;
		printf("Set status to running\n");
		print_queue(threadqueue);
		setcontext(&(runningnode->t_tcb->Context));
		printf("set context failed\n");
	}
	//Thread Yielded, runningnode is READY
	//Thread Blocked, runningnode is BLOCKED
	//Thread Interrupted (timer), runningnode is RUNNING
	else {
		if (runningnode->t_tcb == RUNNING){ // was interrupted
			runningnode->t_tcb = READY;
		}
		enqueue(threadqueue, runningnode);

		// cannot run a blocked thread
		runningnode = dequeue(threadqueue);
		while(runningnode->t_tcb->Status == BLOCKED){
			enqueue(threadqueue, runningnode);
			runningnode = dequeue(threadqueue);
		}
		runningnode->t_tcb->Status = RUNNING;
		setcontext(&(runningnode->t_tcb->Context));
	}
}

void free_queue_node(my_queue_node* finishednode){
	if(finishednode == NULL){
		return;
	}
	printf("freeing node id:%d Status:%d\n", finishednode->t_tcb->Id, finishednode->t_tcb->Status);

	printf("Freeing stack\n");
	if(finishednode->t_tcb->Context.uc_stack.ss_sp != NULL){
		free(finishednode->t_tcb->Context.uc_stack.ss_sp); // deallocate all memory for queue node	
	}
	printf("Freeing TCB\n");
	if(finishednode->t_tcb != NULL){
		free(finishednode->t_tcb);
	}
	printf("Freeing node\n");
	if(finishednode != NULL){
		free(finishednode);	
	}
	printf("Done freeing\n");
	return;
}

my_queue_node* find_node(mypthread_t thread){ // look through queue to find the queue node corresponding to a thread
	my_queue_node* ptr = threadqueue->first;
	while(ptr != NULL){
		if(ptr->t_tcb->Id == thread){
			return ptr; // returns node containing tcb with corresponding thread id
		}
		else{
			ptr = ptr->next;
		}
	}
	return NULL; // not found
}

void enqueue(my_queue* queue_list, my_queue_node* node){
	if (isEmpty(queue_list)) {
		queue_list->first = node;
		queue_list->last = node;
	}
	else {
		my_queue_node* last = queue_list->last;
		last->next = node;
		queue_list->last = node;
	}
}

my_queue_node* dequeue(my_queue* queue){
	//	last<-...<-...<-first
	if (isEmpty(queue)) { // empty
		return NULL;
	}
	else if (queue->first == queue->last){ // one item
		my_queue_node * dequeued = queue->first;
		queue->first = NULL;
		queue->last = NULL;
		return dequeued;
	}
	else { // multiple items
		my_queue_node * dequeued = queue->first;
		my_queue_node * newfirst = dequeued->next;
		queue->first = newfirst;
		return dequeued;
	}
}

my_queue_node* psjf_dequeue(my_queue* queue){
	if(isEmpty(queue)){
		retrun NULL;
	}
	else if(queue->first == queue->last){
		my_queue_node * dequeued = queue->first;
		queue->first = NULL;
		queue->last = NULL;
		return dequeued;
	}
	else { // multiple items
		my_queue_node* dequeued = queue->first;
		my_queue_node* ptr = queue->first;
		while(ptr != NULL){
			if(ptr->t_tcb->Priority < dequeued->t_tcb->Priority){
				dequeued = ptr;
			}
			ptr = ptr->next;
		}

		my_queue_node* prevnode = get_prev_node(queue, dequeued);

		if(dequeued->next == NULL){
			prevnode->next = NULL;
		}
		else{
			prevnode->next = dequeued->next;
		}
		return dequeued;
	}
	return NULL;
}

my_queue_node* get_prev_node(my_queue* queue, my_queue_node* node){

	my_queue_node* ptr = queue->first;

	if(isEmpty(queue)){
		return NULL;
	}

	if(queue->first == node){
		return NULL;
	}

	while(ptr->next != NULL){
		if(ptr->next == node){
			return ptr;
		}
		ptr = ptr->next;
	}
	return NULL;
}

int isEmpty(my_queue* queue){
	if(queue->first == NULL && queue->last == NULL){
		return 1;
	}
	else {
		return 0;
	}
}

int allBlocked(my_queue* queue){
	if(queue == NULL){
		printf("Queue is uninitialized\n");
		return -1;
	}
	my_queue_node * ptr = queue->first;
	while(ptr != NULL){
		if(ptr->t_tcb->Status != BLOCKED){ // READY
			return 0;
		}
		ptr = ptr->next;
	}
	return 1;
}

// void print_queue(queue* queue_print){
// 	if(queue_print == NULL){
// 		printf("Queue is empty\n");
// 		return;
// 	}
// 	printf("before ptr\n");
// 	queue_node * ptr = queue_print->first;
// 	while(ptr != NULL){
// 		printf("ptr\n");
// 		tcb * test = ptr->t_tcb;
// 		printf("ID: %d Status: %d Runtime: %d\n", test->Id, test->Status, (int)(test->TimeQuantums));
// 		ptr = ptr->next;
// 	}
// }

void print_queue(my_queue* queue_print){
	if(queue_print == NULL){
		printf("Queue is uninitialized\n");
		return;
	}
	printf("before ptr\n");
	my_queue_node * ptr = queue_print->first;
	while(ptr != NULL){
		printf("ptr\n");
		// tcb * test = ptr->t_tcb;
		printf("ID: %d Status: %d Runtime: %d\n", ptr->t_tcb->Id, ptr->t_tcb->Status, (int)(ptr->t_tcb->TimeQuantums));
		ptr = ptr->next;
	}
	printf("printing is done\n");
}

void processFinishedJob(int tID){

}

my_mutex_node* find_mutex(int mutexid){
	ignore_int = 1;

	my_mutex_node* mptr = mutexlist;
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

my_mutex_node* find_prev_mutex(int mutexid){
	ignore_int = 1;

	my_mutex_node* mptr = mutexlist;
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

void timer_ended(){
	swapcontext(&(runningnode->t_tcb->Context), &schedulerContext);
}

void start_timer(){
	timer.it_value.tv_sec = TIME_QUANTUM/1000;
	timer.it_value.tv_usec = 0;
	timer.it_interval = timer.it_value;

	setitimer(ITIMER_REAL, &timer, NULL);

	myAction.sa_handler = &timer_ended;
	sigaction(SIGALRM, &myAction, NULL);
}
