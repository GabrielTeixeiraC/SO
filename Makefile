CC = gcc

INC = .

OBJS = main.o dlist.o dccthread.o
HDRS = $(INC)/dlist.h $(INC)/dccthread.h

CFLAGS = -Wall -g -c -I$(INC)

EXE = main

all: $(EXE)

main: $(OBJS)
	$(CC) -o main $(OBJS)

main.o: $(HDRS) main.c
	$(CC) $(CFLAGS) -o main.o main.c

dlist.o: $(HDRS) dlist.c
	$(CC) $(CFLAGS) -o dlist.o dlist.c

dccthread.o: $(HDRS) dccthread.c
	$(CC) $(CFLAGS) -o dccthread.o dccthread.c

clean:
	rm -f *.o $(EXE)