#include <stdio.h>
#include <stdlib.h>
#include "dlist.h"

int main(int argc, char *argv[])
{
    struct dlist *dl = dlist_create();

    int *a = malloc(sizeof(int));
    int *b = malloc(sizeof(int));
    int *c = malloc(sizeof(int));

    *a = 10;
    *b = 20;
    *c = 30;

    dlist_push_right(dl, a);
    dlist_push_right(dl, b);
    dlist_push_right(dl, c);

    struct dnode *node = dl->head;
    while (node != NULL) {
        printf("%d\n", *(int *)node->data);
        node = node->next;
    }

    dlist_destroy(dl, free);
    return 0;
}
