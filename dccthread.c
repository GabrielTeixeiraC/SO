#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include "dlist.h"
#include "dccthread.h"

#define STACK_SIZE 1024*1024
#define MAX_THREAD_NAME_SIZE 128

struct dccthread {
	char name[MAX_THREAD_NAME_SIZE];
	ucontext_t context;
	int yielded;
	int waiting;
};

ucontext_t manager;
struct dlist *threads;

void dccthread_init(void (*func)(int), int param) {
	threads = dlist_create();
	dccthread_create("main", func, param);
	getcontext(&manager);

	while (!dlist_empty(threads)) {
		dccthread_t *thread = dlist_get_index(threads, 0);

		if (thread->waiting) {
			dlist_pop_left(threads);
			dlist_push_right(threads, thread);
			continue;
		}

		swapcontext(&manager, &thread->context);
		dlist_pop_left(threads);

		if (thread->yielded || thread->waiting) {
			thread->yielded = 0;
			dlist_push_right(threads, thread);
		}
	}

	exit(0);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
	dccthread_t *thread = malloc(sizeof(dccthread_t));
	
	strcpy(thread->name, name);
	thread->yielded = 0;
	thread->waiting = 0;
	
	ucontext_t context;
	getcontext(&context);
	context.uc_link = &manager;
	context.uc_stack.ss_sp = malloc(STACK_SIZE);
	context.uc_stack.ss_size = STACK_SIZE;

	thread->context = context;

	dlist_push_right(threads, thread);

	makecontext(&thread->context, (void (*)()) func, 1, param);

	return thread;

}

void dccthread_yield(void) {
	dccthread_t *current_thread = dccthread_self();
	current_thread->yielded = 1;
	swapcontext(&current_thread->context, &manager);
}

void dccthread_exit(void) {
}

void dccthread_wait(dccthread_t *tid) {
}

void dccthread_sleep(struct timespec ts) {
	// TODO
}

dccthread_t * dccthread_self(void) {
	return dlist_get_index(threads, 0);
}

const char * dccthread_name(dccthread_t *tid) {
	return tid->name;
}