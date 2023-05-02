#include <stdio.h>
#include "dlist.h"

int main(int argc, char *argv[])
{
  struct dlist *dl = dlist_create();
  printf("dl->count = %d\n", dl->count);
  return 0;
}
