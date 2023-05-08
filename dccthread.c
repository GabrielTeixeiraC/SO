#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "dlist.h"
#include "dccthread.h"

#define STACK_SIZE 1024*1024
#define MAX_THREAD_NAME_SIZE 128
#define TIMER_INTERVAL_SEC 0
#define TIMER_INTERVAL_NSEC 10000000

struct dccthread {
	char name[MAX_THREAD_NAME_SIZE];
	ucontext_t * context;
	dccthread_t * waited_thread;
};

ucontext_t manager;
struct dlist * threads;
struct dlist * sleeping_threads;
sigset_t mask;

dccthread_t * current_thread;

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
}

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
  
  ts.it_interval.tv_sec = TIMER_INTERVAL_SEC;
  ts.it_interval.tv_nsec = TIMER_INTERVAL_NSEC;
  ts.it_value.tv_sec = TIMER_INTERVAL_SEC;
  ts.it_value.tv_nsec = TIMER_INTERVAL_NSEC;
  
	sigaction(SIGRTMIN, &sa, NULL);

	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id);

	timer_settime(timer_id, 0, &ts, NULL);

	printf("threads: %d, sleeping_threads: %d, thread %s running\n", threads->count, sleeping_threads->count, dccthread_name(dccthread_self()));
	while (!dlist_empty(threads) || !dlist_empty(sleeping_threads)) {
		dccthread_t * current_thread = dlist_get_index(threads, 0);

		if (current_thread->waited_thread != NULL) {
			dlist_push_right(threads, current_thread);
			dlist_pop_left(threads);
			continue;
		}

		swapcontext(&manager, current_thread->context);
		dlist_pop_left(threads);

		if (current_thread->waited_thread != NULL) {
			dlist_push_right(threads, current_thread);
		}
		print_all_threads();
	}

	timer_delete(timer_id);

	dlist_destroy(threads, NULL);
	dlist_destroy(sleeping_threads, NULL);

	exit(0);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
	ucontext_t * context = malloc(sizeof(ucontext_t));
	getcontext(context);

	sigprocmask(SIG_BLOCK, &mask, NULL);
	context->uc_link = &manager;
	context->uc_stack.ss_sp = malloc(STACK_SIZE);
	context->uc_stack.ss_size = STACK_SIZE;

	dccthread_t *thread = malloc(sizeof(dccthread_t));

	strcpy(thread->name, name);
	thread->context = context;
	thread->waited_thread = NULL;
	dlist_push_right(threads, thread);

	makecontext(thread->context, (void (*)()) func, 1, param);

	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	return thread;
}

void dccthread_yield(void) {
	sigprocmask(SIG_BLOCK, &mask, NULL);
	dccthread_t * current_thread = dlist_get_index(threads, 0);
	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_exit(void) {
	sigprocmask(SIG_BLOCK, &mask, NULL);
	dccthread_t * current_thread = dlist_get_index(threads, 0);

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
	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_wait(dccthread_t *tid) {
	sigprocmask(SIG_BLOCK, &mask, NULL);

	if (!find_thread_in_lists(tid)) {
		return;
	}
	
	dccthread_t * current_thread = dlist_get_index(threads, 0);
	current_thread->waited_thread = tid;	
	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int cmp (const void * a, const void * b, void * param) {
	dccthread_t *thread_a = (dccthread_t *) a;
	dccthread_t *thread_b = (dccthread_t *) b;

	return (thread_a != thread_b);
}

void dccthread_wakeup() {
	sigprocmask(SIG_BLOCK, &mask, NULL);

	// struct dnode *node = sleeping_threads->head; 
	// while (node != NULL) {
	// 	dccthread_t *thread = node->data;
	// 	if (thread == (dccthread_t *)si->si_value.sival_ptr){
	// 		dlist_find_remove(sleeping_threads, node, cmp, NULL);
	// 		dlist_push_right(threads, thread);
	// 		break;
	// 	}
	// 	node = node->next;
	// }
	dccthread_t *sleeping_thread = dlist_get_index(sleeping_threads, 0);

	dlist_pop_left(sleeping_threads);
	dlist_push_right(threads, sleeping_thread);


	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_sleep(struct timespec ts) {
	sigprocmask(SIG_BLOCK, &mask, NULL);

	dccthread_t * current_thread = dlist_get_index(threads, 0);

	timer_t st;
	struct sigevent sse;
	struct sigaction ssa;
	struct itimerspec sts;

	ssa.sa_flags = SA_SIGINFO;
	ssa.sa_sigaction = dccthread_wakeup;
	ssa.sa_mask = mask;
	sigaction(SIGRTMAX, &ssa, NULL);

	sse.sigev_notify = SIGEV_SIGNAL;
	sse.sigev_signo = SIGRTMAX;
	sse.sigev_value.sival_ptr = current_thread;
	timer_create(CLOCK_REALTIME, &sse, &st);

	sts.it_value = ts;
	sts.it_interval.tv_sec = 0;
	sts.it_interval.tv_nsec = 0;
	timer_settime(st, 0, &sts, NULL);

	dlist_push_right(sleeping_threads, current_thread);

	swapcontext(current_thread->context, &manager);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

dccthread_t * dccthread_self(void) {
	return dlist_get_index(threads, 0);
}

const char * dccthread_name(dccthread_t *tid) {
	return tid->name;
}