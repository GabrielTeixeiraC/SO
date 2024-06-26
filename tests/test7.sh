#!/bin/bash
set -u

i=7

gcc -g -Wall -I. tests/test$i.c dccthread.o dlist.o -o test$i -lrt &>> gcc.log
if [ ! -x test$i ] ; then
    echo "[$i] compilation error"
    exit 1 ;
fi

./test$i > test$i.out 2> test$i.err

if ! diff tests/test$i.out test$i.out &> /dev/null ; then
    echo "[$i] output for test$i does not match"
    exit 1
fi

exit 0
