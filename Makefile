CC = gcc

INC = .

OBJS = main.o dlist.o
HDRS = $(INC)/dlist.h

CFLAGS = -Wall -g -c -I$(INC)

EXE = main

all: $(EXE)

main: $(OBJS)
	$(CC) -o main $(OBJS)

main.o: $(HDRS) main.c
	$(CC) $(CFLAGS) -o main.o main.c

dlist.o: $(HDRS) dlist.c
	$(CC) $(CFLAGS) -o dlist.o dlist.c

clean:
	rm -f *.o $(EXE)