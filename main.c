#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

ucontext_t context1, context2;

void func1(void)
{
    printf("Function 1 running...\n");

    getcontext(&context1);
    // Switch to context 2
    printf("Switching to context 2...\n");
    setcontext(&context2);

    printf("Function 1 exiting...\n");
}

void func2(void)
{
    printf("Function 2 running...\n");

    getcontext(&context2);
    // Switch back to context 1
    printf("Switching to context 1...\n");
    setcontext(&context1);

    printf("Function 2 exiting...\n");
}

int main(void)
{
    char *stack1 = malloc(1024*1024);
    char *stack2 = malloc(1024*1024);

    // Initialize context 1
    getcontext(&context1);
    context1.uc_stack.ss_sp = stack1;
    context1.uc_stack.ss_size = 1024*1024;
    context1.uc_link = &context2;
    makecontext(&context1, (void (*)()) func1, 0);

    // Initialize context 2
    getcontext(&context2);
    context2.uc_stack.ss_sp = stack2;
    context2.uc_stack.ss_size = 1024*1024;
    context2.uc_link = &context1;
    makecontext(&context2, (void (*)()) func2, 0);

    // Switch to context 1
    setcontext(&context1);

    // Free stacks
    free(stack1);
    free(stack2);

    printf("Exiting...\n");
    return 0;
}
