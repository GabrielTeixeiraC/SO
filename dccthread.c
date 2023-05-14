#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "dlist.h"
#include "dccthread.h"

#define STACK_SIZE 128*128
#define MAX_THREAD_NAME_SIZE 200
#define TIMER_SEC_INTERVAL 0
#define TIMER_NSEC_INTERVAL 10000000

// Thread structure
struct dccthread {
	char name[MAX_THREAD_NAME_SIZE];
	ucontext_t * context;
	dccthread_t * waited_thread;
	int yielded;
};

// Global variables
ucontext_t manager;

struct dlist * threads;
struct dlist * sleeping_threads;

timer_t st;

sigset_t mask;
sigset_t sleepmask;

void print_all_threads() {
	printf("threads: %d, sleeping_threads: %d\n", threads->count, sleeping_threads->count);
	for (int i = 0; i < threads->count; i++) {
		dccthread_t *thread = dlist_get_index(threads, i);
		printf("thread %s\n", thread->name);
	}
	printf("______________________________________\n");
	for (int i = 0; i < sleeping_threads->count; i++) {
		dccthread_t *thread = dlist_get_index(sleeping_threads, i);
		printf("sleeping thread %s\n", thread->name);
	}
	printf("\n\n");
}

// Returns 1 if thread is in threads or sleeping_threads, 0 otherwise
int find_thread_in_lists(dccthread_t * thread) {
	struct dnode * node = threads->head;
	while (node != NULL) {
		if (node->data == thread) {
			return 1;
		}
		node = node->next;
	}

	node = sleeping_threads->head;
	while (node != NULL) {
		if (node->data == thread) {
			return 1;
		}
		node = node->next;
	}

	return 0;
}

// Initializes the thread library
void dccthread_init(void (*func)(int), int param) {
  threads = dlist_create();
  sleeping_threads = dlist_create();
  dccthread_create("main", func, param);
  getcontext(&manager);

  timer_t timer_id;
  struct sigevent se;
  struct sigaction sa;
  struct itimerspec ts;
  
  se.sigev_signo = SIGRTMIN;
  se.sigev_notify = SIGEV_SIGNAL;
  se.sigev_notify_attributes = NULL;
  se.sigev_value.sival_ptr = &timer_id;
  
  sa.sa_flags = 0;
  sa.sa_handler = (void *) dccthread_yield;
  
  ts.it_interval.tv_sec = TIMER_SEC_INTERVAL;
  ts.it_interval.tv_nsec = TIMER_NSEC_INTERVAL;
  ts.it_value.tv_sec = TIMER_SEC_INTERVAL;
  ts.it_value.tv_nsec = TIMER_NSEC_INTERVAL;
  
	sigaction(SIGRTMIN, &sa, NULL);

	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN);
  sigemptyset(&sleepmask);
  sigaddset(&sleepmask, SIGALRM);

	sigprocmask(SIG_BLOCK, &mask, NULL);

	if (timer_create(CLOCK_REALTIME, &se, &timer_id) != 0) {
		perror("timer_create");
		exit(1);
	}	

	if (timer_settime(timer_id, 0, &ts, NULL) != 0) {
		perror("timer_settime");
		exit(1);
	}

	while (!dlist_empty(threads) || !dlist_empty(sleeping_threads)) {
    sigprocmask(SIG_UNBLOCK, &sleepmask, NULL);
    sigprocmask(SIG_BLOCK, &sleepmask, NULL);
  	dccthread_t * current_thread = dccthread_self();
  
		if (current_thread->waited_thread != NULL) {
			dlist_push_right(threads, current_thread);
			dlist_pop_left(threads);
			continue;
		}

		swapcontext(&manager, current_thread->context);
		dlist_pop_left(threads);

		if (current_thread->waited_thread != NULL || current_thread->yielded) {
			current_thread->yielded = 0;
			dlist_push_right(threads, current_thread);
		}
	}

	if (timer_delete(timer_id) != 0) {
		perror("preemption timer delete");
		exit(1);
	}

	if (timer_delete(st) != 0) {
		perror("sleep timer delete");
		exit(1);
	}

	dlist_destroy(threads, NULL);
	dlist_destroy(sleeping_threads, NULL);

	exit(0);
}

// Creates a new thread
dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
	ucontext_t * context = malloc(sizeof(ucontext_t));
	if (context == NULL) {
		perror("context malloc error");
		exit(1);
	}

	getcontext(context);

	sigprocmask(SIG_BLOCK, &mask, NULL);

	context->uc_link = &manager;
	context->uc_stack.ss_sp = malloc(STACK_SIZE);
	if (context->uc_stack.ss_sp == NULL) {
		perror("stack malloc error");
		exit(1);
	}
	context->uc_stack.ss_size = STACK_SIZE;
	dccthread_t *thread = malloc(sizeof(dccthread_t));
	if (thread == NULL) {
		perror("thread malloc error");
		exit(1);
	}

	strcpy(thread->name, name);
	thread->context = context;
	thread->waited_thread = NULL;
	thread->yielded = 0;
	dlist_push_right(threads, thread);

	makecontext(thread->context, (void *) func, 1, param);

	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	return thread;
}

// Yields the current thread
void dccthread_yield(void) {
	sigprocmask(SIG_BLOCK, &mask, NULL);
	dccthread_t * current_thread = dccthread_self();
	current_thread->yielded = 1;
	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

// Exits the current thread
void dccthread_exit(void) {
	sigprocmask(SIG_BLOCK, &mask, NULL);
	dccthread_t * current_thread = dccthread_self();

	for (int i = 0; i < threads->count; i++) {
		dccthread_t *thread = dlist_get_index(threads, i);
		if (thread->waited_thread == current_thread) {
			thread->waited_thread = NULL;
		}
	}
	
	for (int i = 0; i < sleeping_threads->count; i++) {
		dccthread_t *thread = dlist_get_index(sleeping_threads, i);
		if (thread->waited_thread == current_thread) {
			thread->waited_thread = NULL;
		}
	}
	
	free(current_thread);
	setcontext(&manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

// Waits for the given thread to exit
void dccthread_wait(dccthread_t *tid) {
	sigprocmask(SIG_BLOCK, &mask, NULL);

	if (!find_thread_in_lists(tid)) {
		return;
	}
	
	dccthread_t * current_thread = dccthread_self();
	current_thread->waited_thread = tid;	
	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

// Compares two threads
int cmp (const void * a, const void * b, void * param) {
	dccthread_t *thread_a = (dccthread_t *) a;
	dccthread_t *thread_b = (dccthread_t *) b;

	return (thread_a != thread_b);
}

// Wakes up the given thread
void dccthread_wake(int sig, siginfo_t *info, void *ucontext) {
	dccthread_t *thread = (dccthread_t *) info->si_value.sival_ptr;
	dlist_find_remove(sleeping_threads, thread, cmp, NULL);
	dlist_push_right(threads, thread);
}

// Puts the current thread to sleep
void dccthread_sleep(struct timespec ts) {
	sigprocmask(SIG_BLOCK, &mask, NULL);

	dccthread_t * current_thread = dccthread_self();

	struct sigevent sse;
	struct sigaction ssa;
	struct itimerspec sts;

	ssa.sa_flags = SA_SIGINFO;
	ssa.sa_sigaction = dccthread_wake;
	ssa.sa_mask = mask;
	sigaction(SIGALRM, &ssa, NULL);

	sse.sigev_notify = SIGEV_SIGNAL;
	sse.sigev_signo = SIGALRM;
	sse.sigev_value.sival_ptr = current_thread;
	
	if (timer_create(CLOCK_REALTIME, &sse, &st) != 0) {
		perror("timer_create");
		exit(1);
	}

	sts.it_value = ts;
	sts.it_interval.tv_sec = 0;
	sts.it_interval.tv_nsec = 0;
	
	if (timer_settime(st, 0, &sts, NULL) != 0) {
		perror("timer_settime");
		exit(1);
	}

	dlist_push_right(sleeping_threads, current_thread);

	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

// Returns the current thread
dccthread_t * dccthread_self(void) {
	return dlist_get_index(threads, 0);
}

// Returns the name of the given thread
const char * dccthread_name(dccthread_t *tid) {
	return tid->name;
}