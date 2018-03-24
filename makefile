# makefile for project marking problem 
# use: make [producer|pipeline|clean]

SRC=demo.c

CC=gcc -g -std=gnu99 -Wall -Werror
LIB=-lpthread -lrt
LB =-pthread

demo: demo.o
	$(CC) demo.o -o demo 

demo.o: demo.c
	$(CC) -pthread -c demo.c 

readingroom: readingroom.o
	$(CC) readingroom.o -o readingroom

readingroom.o: readingroom.c
	$(CC) -c $(LB) readingroom.c

clean:
	rm demo *.o readingroom 
