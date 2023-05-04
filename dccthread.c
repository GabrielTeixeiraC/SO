#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include "dlist.h"
#include "dccthread.h"

#define STACK_SIZE 1024*1024
#define MAX_THREAD_NAME_SIZE 128

struct dccthread {
	ucontext_t context;
	char name[MAX_THREAD_NAME_SIZE];
};

ucontext_t manager;

void dccthread_init(void (*func)(int), int param) {
	dccthread_t *thread = dccthread_create("main", func, param);
	getcontext(&manager);

	swapcontext(&manager, &thread->context);


	exit(0);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
	dccthread_t *thread = malloc(sizeof(dccthread_t));
	
	strcpy(thread->name, name);
	
	ucontext_t context;
	getcontext(&context);
	context.uc_link = &manager;
	context.uc_stack.ss_sp = malloc(STACK_SIZE);
	context.uc_stack.ss_size = STACK_SIZE;

	thread->context = context;

	makecontext(&thread->context, (void (*)()) func, 1, param);

	return thread;

}
void dccthread_yield(void) {
	// TODO
}
void dccthread_exit(void) {
	// TODO
}
void dccthread_wait(dccthread_t *tid) {
	// TODO
}
void dccthread_sleep(struct timespec ts) {
	// TODO
}
dccthread_t * dccthread_self(void) {
	// TODO
	return NULL;
}
const char * dccthread_name(dccthread_t *tid) {
	return tid->name;
}