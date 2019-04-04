#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void initQueue(Queue *q){
    q->count = 0;
    q->front = 0;
    q->rear = QUEUE_SIZE-1;
}

int isEmpty(Queue *q){
    if(q->count == 0){
        return 1;
    }
    else return 0;
}

int isFull(Queue *q){
    if(q->count == QUEUE_SIZE){
        return 1;
    }
    else return 0;
}

int size(Queue *q){
    return q->count;
}

int push(Queue *q, int data){
    if(!isFull(q)){
        q->rear = (q->rear + 1)%QUEUE_SIZE;
        q->intArray[q->rear] = data;
        q->count++;
        return 1;
    }
    else return 0;
}

int pop(Queue *q){
    if(!isEmpty(q)){
        int data = q->intArray[q->front];
        q->front = (q->front + 1)%QUEUE_SIZE;
        q->count--;
        return data;
    }
    else return 0;
}

