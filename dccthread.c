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
#define TIMER_INTERVAL_NSEC 100000000

struct dccthread {
	char name[MAX_THREAD_NAME_SIZE];
	ucontext_t * context;
	int yielded;
	int sleeping;
	dccthread_t * waited_thread;
};

ucontext_t manager;
struct dlist * threads;
int timer_enabled = 0;

void disable_timer() {
	timer_enabled = 0;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_BLOCK, &mask, NULL);
}

void enable_timer() {
	timer_enabled = 1;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_init(void (*func)(int), int param) {
	threads = dlist_create();
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

	disable_timer();

  if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id) == -1) {
    perror("Error creating timer");
    exit(EXIT_FAILURE);
  }

  if (timer_settime(timer_id, 0, &ts, NULL) == -1) {
    perror("Error setting timer");
    exit(EXIT_FAILURE);
  }

	while (!dlist_empty(threads)) {
		dccthread_t *thread = dlist_get_index(threads, 0);

		if (thread->waited_thread != NULL || thread->sleeping) {
			dlist_pop_left(threads);
			dlist_push_right(threads, thread);
			continue;
		}

		swapcontext(&manager, thread->context);
		dlist_pop_left(threads);

		if (thread->yielded || thread->waited_thread != NULL) {
			thread->yielded = 0;
			dlist_push_right(threads, thread);
		}
	}

	if (timer_delete(timer_id) == -1) {
    perror("Error deleting timer");
    exit(EXIT_FAILURE);
  }

	exit(0);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
	disable_timer();
	
	ucontext_t * context = malloc(sizeof(ucontext_t));
	getcontext(context);
	context->uc_link = &manager;
	context->uc_stack.ss_sp = malloc(STACK_SIZE);
	context->uc_stack.ss_size = STACK_SIZE;

	dccthread_t *thread = malloc(sizeof(dccthread_t));

	strcpy(thread->name, name);
	thread->context = context;
	thread->yielded = 0;
	thread->sleeping = 0;
	thread->waited_thread = NULL;

	dlist_push_right(threads, thread);

	makecontext(thread->context, (void (*)()) func, 1, param);

	enable_timer();

	return thread;
}

void dccthread_yield(void) {
	disable_timer();

	dccthread_t *current_thread = dccthread_self();
	current_thread->yielded = 1;

	enable_timer();
	swapcontext(current_thread->context, &manager);
}

void dccthread_exit(void) {
	disable_timer();

	dccthread_t *current_thread = dccthread_self();
	struct dnode *node = threads->head;

	while (node != NULL) {
		dccthread_t *thread = node->data;
		if (thread->waited_thread == current_thread) {
			thread->waited_thread = NULL;
		}
		node = node->next;
	}
	
	free(current_thread);

	enable_timer();
}

void dccthread_wait(dccthread_t *tid) {
	disable_timer();

	struct dnode *node = threads->head;

	while (node != NULL) {
		dccthread_t *thread = node->data;
		if (thread == tid) {
			break;
		}
		node = node->next;
	}

	if (node == NULL) {
		return;
	}

	dccthread_t *current_thread = dccthread_self();
	current_thread->waited_thread = tid;

	enable_timer();
	swapcontext(current_thread->context, &manager);
}

void dccthread_sleep(struct timespec ts) {
}

dccthread_t * dccthread_self(void) {
	return dlist_get_index(threads, 0);
}

const char * dccthread_name(dccthread_t *tid) {
	return tid->name;
}