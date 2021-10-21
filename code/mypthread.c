// File:	mypthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "mypthread.h"

/*
TODO: DEAL WITH MAIN CONTEXT
TODO: DEAL WITH SCHEDULER CONTEXT
*/

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
int ignore_int = 0;
int init = 0;
int num_thread = 0;
int num_mutex = 0;
int yielded=0;
ucontext_t schedulerContext;
ucontext_t mainContext;
ucontext_t exitContext;
// ucontext_t parentContext;
my_queue* threadqueue = NULL;
my_multi_queue* multiqueue = NULL;
my_queue_node* runningnode = NULL;
my_mutex_node* mutexlist = NULL;
mypthread_mutex_t queuelock;
struct itimerval timer;
struct sigaction myAction;
struct itimerval period_s_timer;
int allBlocked(my_queue* queue);


/* Extra function declarations */
my_queue_node* find_node(my_queue* aqueue, mypthread_t thread);
void enqueue(my_queue* queue, my_queue_node* queue_node);
my_queue_node* dequeue(my_queue* queue);
void free_queue_node(my_queue_node* runningnode);
static void schedule();
my_mutex_node* find_mutex(int mutexid);
my_mutex_node* find_prev_mutex(int mutexid);
static void sched_fifo();
void print_queue(my_queue* queue_print);
my_queue_node* stcf_dequeue(my_queue* queue);
my_queue_node* get_prev_node(my_queue* queue, my_queue_node* node);
my_queue_node* find_node_multiqueue(my_multi_queue* amultiqueue, mypthread_t athread);
int get_time_spent_micro_sec(struct itimerval atimer);
static void sched_stcf();
void mlfq_move_all_to_top(my_multi_queue* amultiqueue);
my_queue_node* mlfq_dequeue(my_multi_queue* amultiqueue);
void start_timer_mlfq(my_queue* aqueue);
void start_timer_period_s();
int isEmpty(my_queue* queue);
void exitfun();
void mutex_unblock_next(mypthread_mutex_t *mutex);

/* Extra function declarations end */


void exitfun(){
	runningnode->t_tcb->Status = EXIT;
	
}


/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// create Thread Control Block
	// create and initialize the context of this thread
	// allocate space of stack for this thread to run
	// after everything is all set, push this thread int
	// YOUR CODE HERE

	printf("Mypthread create was called.\n");

	ignore_int = 1;
	num_thread ++;
	*thread = num_thread;

	if(!init){ // not initialized, make scheduler, exit and main context; make queue/multi_queue depending on the scheduler algo;
		//scheduler context
		getcontext(&schedulerContext);
		schedulerContext.uc_link = NULL;
		schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		schedulerContext.uc_stack.ss_size = STACK_SIZE;

		if(schedulerContext.uc_stack.ss_sp == 0){
			printf("Couldn't allocate space for scheduler context\n");
			exit(1);
		}

		schedulerContext.uc_stack.ss_flags = 0;
		makecontext(&schedulerContext, (void*)&schedule, 0);

		//main context
		getcontext(&mainContext);
		mainContext.uc_link = NULL;
		mainContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		mainContext.uc_stack.ss_size = STACK_SIZE;

		if(mainContext.uc_stack.ss_sp == 0){
			printf("Couldn't allocate space for main context\n");
			exit(1);
		}

		mainContext.uc_stack.ss_flags = 0;


		//exit context

		getcontext(&exitContext);
		exitContext.uc_link = &schedulerContext;
		exitContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		exitContext.uc_stack.ss_size = STACK_SIZE;

		if(exitContext.uc_stack.ss_sp == 0){
			printf("Couldn't allocate space for exit context\n");
			exit(1);
		}

		exitContext.uc_stack.ss_flags = 0;
		makecontext(&exitContext, (void*)&exitfun, 0);


		threadControlBlock* main_tcb = (threadControlBlock*) malloc(sizeof(threadControlBlock));
		main_tcb->Priority = 0;
		main_tcb->TimeQuantums = 0;
		main_tcb->Id = 0;
		main_tcb->Status = READY;
		main_tcb->blocked_by = NULL;
		main_tcb->next_blocked = NULL;
		main_tcb->time_passed = 0;
		main_tcb->return_value = NULL;
		main_tcb->Context = mainContext;

		my_queue_node * mainnode = (my_queue_node*) malloc(sizeof(my_queue_node));
		mainnode->t_tcb = main_tcb;
	
		printf("Creates queue for first time\n");

		if(SCHED != MLFQ_SCHEDULER){
			printf("Using STCF\n");
			threadqueue = (my_queue*) malloc(sizeof(my_queue));
			threadqueue->first = mainnode;
			threadqueue->last = mainnode;
			threadqueue->timeslice = 0;
		}
		else{ //MLFQ
			printf("Using MLFQ\n");
			multiqueue = (my_multi_queue*) malloc(sizeof(my_multi_queue));
			multiqueue->queue_arr = (my_queue**) malloc((LOWEST_PRIORITY+1) * sizeof(my_queue*));
			multiqueue->queue_arr[0]->first = mainnode;
			multiqueue->queue_arr[0]->last = mainnode;
			multiqueue->queue_arr[0]->timeslice = 2;
			for(int i = 1; i <= LOWEST_PRIORITY; i++){
				multiqueue->queue_arr[i]->first = NULL;
				multiqueue->queue_arr[i]->last = NULL;
				multiqueue->queue_arr[i]->timeslice = multiqueue->queue_arr[i-1]->timeslice+2;
			}
		}
		// mypthread_mutex_init(&queuelock, NULL);
	}
	
	threadControlBlock* new_tcb = (threadControlBlock*) malloc(sizeof(threadControlBlock));
	new_tcb->Priority = 0;
	new_tcb->TimeQuantums = 0;
	new_tcb->Id = *thread;
	new_tcb->Status = READY;
	new_tcb->blocked_by = NULL;
	new_tcb->next_blocked = NULL;
	new_tcb->time_passed = 0;
	new_tcb->return_value = NULL;

	ucontext_t newThreadContext;
	getcontext(&newThreadContext);
	newThreadContext.uc_link = &exitContext;
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
	printf("This is node: %u\n", new_tcb->Id);

	if(SCHED != MLFQ_SCHEDULER){
		enqueue(threadqueue, qnode);
	}
	else { //MLFQ
		enqueue(multiqueue->queue_arr[0], qnode);
	}

	//print_queue(threadqueue);
	init = 1;
	ignore_int = 0;
	swapcontext(&mainContext, &schedulerContext);
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
	free_queue_node(ptr); // deallocate memory - need to eventually figure this out
	printf("freed node\n");
	setcontext(&schedulerContext); // go to scheduler
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	my_queue_node* node;
	if(SCHED == MLFQ_SCHEDULER){
		node = find_node_multiqueue(multiqueue, thread);
	}
	else{
		node = find_node(threadqueue, thread);
	}
	 // find blocking node
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
	mutex_unblock_next(mutex);

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
	printf("scheduler being called\n");
	// if(isEmpty(threadqueue)){
	// 	setcontext(&mainContext);
	// }

	// schedule policy
	#ifdef MLFQ
		sched_MLFQ();
	#else
		sched_stcf();
	#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)
	printf("STCF called\n");
	
	if(runningnode == NULL){
		runningnode = stcf_dequeue(threadqueue);
		if (runningnode == NULL){
			return;
		}
		printf("dequeued node\n");
		printf("runnning node is: %u\n", runningnode->t_tcb->Id);

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
		if (runningnode->t_tcb->Status == RUNNING){ // was interrupted
			runningnode->t_tcb->Status = READY;
		}
		runningnode->t_tcb->TimeQuantums++;
		enqueue(threadqueue, runningnode);

		// cannot run a blocked thread
		runningnode = stcf_dequeue(threadqueue);
		if (runningnode == NULL){
			return;
		}
		runningnode->t_tcb->Status = RUNNING;
		printf("set status to running\n");
		print_queue(threadqueue);
		setcontext(&(runningnode->t_tcb->Context));
		printf("set context failed\n");
	}

	// YOUR CODE HERE
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

	// Check if period S has no remaining time from signal. If so, move all jobs to topmost queue.
	if (period_s_timer.it_value.tv_sec == 0 && period_s_timer.it_value.tv_usec == 0){
		mlfq_move_all_to_top(multiqueue);
		start_timer_period_s();
		setcontext(&(runningnode->t_tcb->Context)); // check this
	}

	//Thread Finished, runningnode is NULL - schedule new runningnode starting from queues of most importance
	if(runningnode == NULL){
		my_queue_node * new_running_node = mlfq_dequeue(multiqueue);
		if(new_running_node == NULL){
			return;
		}
		else{
			runningnode = new_running_node;
			runningnode->t_tcb->Status = RUNNING;
			printf("Set status to running\n");
			start_timer_mlfq(multiqueue->queue_arr[runningnode->t_tcb->Priority]);
			setcontext(&(runningnode->t_tcb->Context));
			printf("set context failed\n");
		}
	}
	//Thread Interrupted or Yielded or Blocked, runningnode is RUNNING or READY or BLOCKED
	//YIELDED PROBLEM need to keep track of total spent time, add total time attribute?
	//Thread Blocked, runningnode is BLOCKED - SAME problem as yielded
	else {
		//add to total time passed
		runningnode->t_tcb->time_passed += get_time_spent_micro_sec(timer);

		//Exceeded time slice, increment priority / decrease importance and reset time passed
		if(runningnode->t_tcb->Priority < LOWEST_PRIORITY && runningnode->t_tcb->time_passed > (multiqueue->queue_arr[runningnode->t_tcb->Priority]->timeslice)*1000){
			runningnode->t_tcb->Priority ++;
			runningnode->t_tcb->time_passed = 0;
		}
		// send node to the back of the appropriate queue
		enqueue(multiqueue->queue_arr[runningnode->t_tcb->Priority], runningnode);
		runningnode = NULL;

		// choose with round robin dequeue, taking into account priority and blocked statuses
		my_queue_node * new_running_node = mlfq_dequeue(multiqueue);
		if(new_running_node == NULL){
			// handle no more nodes or deadlock
			return;
		}
		else{
			runningnode = new_running_node;
			runningnode->t_tcb->Status = RUNNING;
			printf("Set status to running\n");
			start_timer_mlfq(multiqueue->queue_arr[runningnode->t_tcb->Priority]);
			setcontext(&(runningnode->t_tcb->Context));
			printf("set context failed\n");
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
		printf("runnning node is: %u\n", runningnode->t_tcb->Id);
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
	printf("freeing node id:%u Status:%d\n", finishednode->t_tcb->Id, finishednode->t_tcb->Status);

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

void mlfq_move_all_to_top(my_multi_queue* amultiqueue){
	int i = 1;
	while(i <= LOWEST_PRIORITY){
		while(!isEmpty(amultiqueue->queue_arr[i])){
			my_queue_node * temp = dequeue(amultiqueue->queue_arr[i]);
			temp->t_tcb->Priority = 0;
			temp->t_tcb->time_passed = 0;
			enqueue(amultiqueue->queue_arr[0], temp);
		}
	}
	return;
}

// special dequeue for multiqueues
my_queue_node* mlfq_dequeue(my_multi_queue* amultiqueue){
	int i = 0;
	while((isEmpty(amultiqueue->queue_arr[i]) || allBlocked(amultiqueue->queue_arr[i])) && i <= LOWEST_PRIORITY){ // get multiqueue queue that is not empty and not all blocked - Rule 1
		i++;
	}
	if(i > LOWEST_PRIORITY){ // no more threads to run or deadlock
		return NULL;
	}
	my_queue_node* ptr = dequeue(amultiqueue->queue_arr[i]);
	printf("dequeued node\n");
	printf("runnning node is: %u\n", runningnode->t_tcb->Id);

	while(runningnode->t_tcb->Status == BLOCKED){
		printf("status is blocked\n");
		enqueue(amultiqueue->queue_arr[i], ptr);
		ptr = dequeue(amultiqueue->queue_arr[i]);
	}

	return ptr;
}

my_queue_node* find_node(my_queue* aqueue, mypthread_t athread){ // look through queue to find the queue node corresponding to a thread
	my_queue_node* ptr = aqueue->first;
	while(ptr != NULL){
		if(ptr->t_tcb->Id == athread){
			return ptr; // returns node containing tcb with corresponding thread id
		}
		else{
			ptr = ptr->next;
		}
	}
	return NULL; // not found
}

my_queue_node* find_node_multiqueue(my_multi_queue* amultiqueue, mypthread_t athread){ // look through queue to find the queue node corresponding to a thread
	my_queue_node* ptr;
	for(int i = 0; i <= LOWEST_PRIORITY; i++){
		ptr = amultiqueue->queue_arr[i]->first;
		while(ptr != NULL){
			if(ptr->t_tcb->Id == athread){
				return ptr; // returns node containing tcb with corresponding thread id
			}
			else{
				ptr = ptr->next;
			}
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
	return;
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

my_queue_node* stcf_dequeue(my_queue* queue){
	if(isEmpty(queue)){
		return NULL;
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
			if((ptr->t_tcb->TimeQuantums < dequeued->t_tcb->TimeQuantums && ptr->t_tcb->Status != BLOCKED) || dequeued->t_tcb->Status == BLOCKED && ptr->t_tcb->Status != BLOCKED){
				dequeued = ptr;
			}
			ptr = ptr->next;
		}

		if(dequeued->t_tcb->Status != BLOCKED){
			my_queue_node* prevnode = get_prev_node(queue, dequeued);

			if(dequeued->next == NULL){
				prevnode->next = NULL;
			}
			else{
				prevnode->next = dequeued->next;
			}
			return dequeued;
		}
		else{
			return NULL;
		}
	}
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

// checks if a queue is full of blocked nodes
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

void print_queue(my_queue* queue_print){
	if(queue_print == NULL){
		printf("Queue is uninitialized\n");
		return;
	}
	printf("before ptr\n");
	my_queue_node * ptr = queue_print->first;
	while(ptr != NULL){
		printf("ptr\n");
		printf("ID: %u Status: %d Runtime: %d\n", ptr->t_tcb->Id, ptr->t_tcb->Status, (int)(ptr->t_tcb->TimeQuantums));
		ptr = ptr->next;
	}
	printf("printing is done\n");
	return;
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

void mutex_unblock_next(mypthread_mutex_t *mutex){
	if(mutex == NULL){
		return;
	}
	if(mutex->node_blocked_list == NULL){
		return;
	}
	my_queue_node* first_blocked = mutex->node_blocked_list;
	mutex->node_blocked_list = first_blocked->next;
	first_blocked->t_tcb->Status = READY;
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

// start timer for mlfq - no difference except for now it uses microseconds
void start_timer_mlfq(my_queue* aqueue){
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = (aqueue->timeslice)*1000;
	timer.it_interval = timer.it_value;

	setitimer(ITIMER_REAL, &timer, NULL);

	myAction.sa_handler = &timer_ended;
	sigaction(SIGALRM, &myAction, NULL);
}

// returns time spent by timer in micro seconds
int get_time_spent_micro_sec(struct itimerval atimer){
	return (atimer.it_interval.tv_sec - atimer.it_value.tv_sec)*1000000 + (atimer.it_interval.tv_usec - atimer.it_value.tv_usec);
}

void start_timer_period_s(){
	period_s_timer.it_value.tv_sec = 0;
	period_s_timer.it_value.tv_usec = TIME_PERIOD_S;
	period_s_timer.it_interval = period_s_timer.it_value;

	setitimer(ITIMER_REAL, &period_s_timer, NULL);

	myAction.sa_handler = &timer_ended;
	sigaction(SIGALRM, &myAction, NULL);
}





