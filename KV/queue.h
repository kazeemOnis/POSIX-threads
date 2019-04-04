#define QUEUE_SIZE 10

struct my_queue {

    int front;
    int rear;
    int count;
    int intArray[QUEUE_SIZE];

};

typedef struct my_queue Queue;

void initQueue(Queue * q);
int isEmpty(Queue * q);		// returns 1 when empty and 0 otherwise
int isFull(Queue * q);		// returns 1 when full and 0 otherwise
int size(Queue * q);		// returns number of items in queue
int push(Queue * q, int data);	//returns 0 on failure  and 1 on success
int pop(Queue * q);		//returns 0 on failure  and integer > 1 on success
