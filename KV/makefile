# makefile for project marking problem 
# use: make 

CC=gcc -g -std=gnu99 -pthread
LIB=-lpthread -lrt
LB =-pthread

server: server.c kv.c queue.c parser.c
	$(CC) server.c kv.c parser.c queue.c -o server 