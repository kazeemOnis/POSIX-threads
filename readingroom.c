#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;   //mutex intialisation
/*condition variables initialisation*/
pthread_cond_t writerDone = PTHREAD_COND_INITIALIZER; //condition variable for when the writer is done
pthread_cond_t roomEmpty = PTHREAD_COND_INITIALIZER;  //condition variable for when the room becomes empty
/*global variables initialisation*/
int roomOccupiedbyWriter = 0;  //variable determining if the room is occupied by a writer
int noInRoom = 0;  //variable used to count to number of people currently in the room
int writerWating = 0; //variable used to determine if a writer is currently waiting
int writerState = 0;  //variable used to determine if the writer is done writing
int readerState = 0;  //variable used to determine if the reader is done reading
/* Do not change this code. */

struct timeval time0;

int now() {
    struct timeval _now;
    gettimeofday(&_now, NULL);
    return (_now.tv_sec - time0.tv_sec) * 100 +
           (_now.tv_usec - time0.tv_usec) / 10000;
}

void xwait(int delay) {
    struct timespec p, q;
    delay *= 10;
    q.tv_sec = delay/1000;
    q.tv_nsec = 1000000 * (delay % 1000);
    int err = nanosleep(&q, &p);
    if (err) { abort(); }
}

void read_documents(int delay, int id) {
    printf("%i: Reader %i reading ...\n", now(), id);
    xwait(delay);
    printf("%i: Reader %i done reading.\n", now(), id);
}

void write_documents(int delay, int id) {
    printf("%i: Writer %i writing ...\n", now(), id);
    xwait(delay);
    printf("%i: Writer %i done writing.\n", now(), id);
}

typedef struct _info {
    int id;
    int delay;
    pthread_t *thread;
} info;

/* End of code to leave alone. */

//reader thread function
void* reader(void* p) {
    info *param = (info*) p;
    info i = *param;
    free(p);
    int err;

    /* Print this line when a new reader is created, before any synchronisation operations. */
    printf("%i: New reader %i (delay=%i).\n", now(), i.id, i.delay);

    //lock the associated mutex
    err = pthread_mutex_lock(&mutex);
    if(err){
        printf("Error locking mutex with errno %d\n",err);
        exit(1);
    }
    /*
    If a writer is currently waiting to enter a room,
    reader must wait for associated condition variable
    */
    if(writerWating == 1){
        /* This line must be printed only if the reader has to wait. */
        printf("%i: Reader %i waiting ...\n", now(), i.id);
        while(writerWating == 1){
            // reader must wait for writer done condition
            err = pthread_cond_wait(&writerDone,&mutex);
            if(err){
                printf("Error waiting for condition variable with errno %d\n",err);
                exit(1);
            }
        }
    }
    /*
    If the room is currently occupied by a writer,
    reader must wait for associated condition variable
    */
    else if(roomOccupiedbyWriter == 1){
         /* This line must be printed only if the reader has to wait. */
        printf("%i: Reader %i waiting ...\n", now(), i.id);
        while(roomOccupiedbyWriter == 1){
            // reader must wait for writer done condition
            err = pthread_cond_wait(&writerDone,&mutex);
            if(err){
                printf("Error waiting for condition variable with errno %d\n",err);
                exit(1);
            }
        }
    }
    /*
    If the room is not occupied by a writer,
    reader can proceed to entering the room
    */
    if(roomOccupiedbyWriter == 0){
       /* Print this line when the reader enters the room. */
        printf("%i: Reader %i enters room.\n", now(), i.id);
        noInRoom++; // increment the number of people in the room
        readerState = 1; // change the reader's state to reflect entering the room
    }
    //unlock the associated mutex
    err = pthread_mutex_unlock(&mutex);
    if(err){
        exit(1);
    }
    /*
    Once the reader has entered the room,
    can proceed to reading documents
    */
    if(readerState == 1){
         /* Execute this line when it is safe to do so. */
        read_documents(i.delay, i.id);
    }

    //lock the associated mutex
    err = pthread_mutex_lock(&mutex);
    if(err){
        printf("Error locking mutex with errno %d\n",err);
        exit(1);
    }
    /* Print this line when the reader leaves the room. */
    printf("%i: Reader %i leaves room.\n", now(), i.id);
    noInRoom--; // decrement the number of people in the room
    readerState = 0;  // change the reader's state to reflect leaving the room
    /*
    If the number of readers in the room is zero,
    signal that the room is empty for waiting writers
    */
    if(noInRoom == 0){
        // reader signals the room empty condition
        err = pthread_cond_signal(&roomEmpty);
        if(err){
            printf("Error signalling condition variable with errno %d\n",err);
            exit(1);
        }
    }
    printf("No of people in room %d\n",noInRoom);
    //unlock the associated mutex
    err = pthread_mutex_unlock(&mutex);
    if(err){
        printf("Error unlocking mutex with errno %d\n",err);
        exit(1);
    }

    return NULL;
}

//writer thread function
void* writer(void* p) {
    info* param = (info*) p;
    info i = *param;
    free(p);
    int err;

    /* Print this line before the first synchronisation operation. */
    printf("%i: New writer %i (delay=%i).\n", now(), i.id, i.delay);

    //lock the associated mutex
    err = pthread_mutex_lock(&mutex);
    if(err){
        printf("Error locking mutex with errno %d\n",err);
        exit(1);
    }
    writerWating = 1;  //initialise that the writer is waiting
    /*
    If the room is currently occupied by another writer,
    wait for the associated condition variable
    */
    if(roomOccupiedbyWriter == 1){
        while(roomOccupiedbyWriter == 1){
             /* Print this line only if the writer has to wait. */
            printf("%i: Writer %i waiting ...\n", now(), i.id);
            // writer must wait for the current writer to be done
            err = pthread_cond_wait(&roomEmpty,&mutex);
            if(err){
                printf("Error waiting for condition variable with errno %d\n",err);
                exit(1);
            }
        }
    }
    /*
    Otherwise if the room is currently not occupied by a writer,
    check if there are any readers in the room
    */
    else{
        //while the room is currently occupied by any number of readers
       while(noInRoom != 0){
            /* Print this line only if the writer has to wait. */
            printf("%i: Writer %i waiting ...\n", now(), i.id);
            //wait for the room to be empty to proceed
            err = pthread_cond_wait(&roomEmpty,&mutex);
            if(err){
                printf("Error waiting for condition variable with errno %d\n",err);
                exit(1);
            }
        }
    }
    /*
    Once the room becomes empty,
    writer can proceed to entering the room
    */
    if(noInRoom == 0){
        /* Print this line when the writer enters the room. */
        printf("%i: Writer %i enters room.\n", now(), i.id);
    }
    roomOccupiedbyWriter = 1; // initialise that the room is occupied by a writer
    writerState = 1; // change the writers state to simulate entering the room
    writerWating = 0; //writer is now done waiting and entered room
    //unlock the associated mutex
    err = pthread_mutex_unlock(&mutex);
    if(err){
        printf("Error unlocking mutex with errno %d\n",err);
        exit(1);
    }
    printf("No in room = %d\n",noInRoom);
    /*
    Once the writer has entered the room,
    can proceed to writing documents
    */
    if(writerState == 1){
         /* Execute this line when it is safe to do so. */
        write_documents(i.delay, i.id);
    }

    //lock the associated mutex
    err = pthread_mutex_lock(&mutex);
    if(err){
        printf("Error locking mutex with errno %d\n",err);
        exit(1);
    }
     /* Print this line when the writer leaves the room. */
    printf("%i: Writer %i leaves room.\n", now(), i.id);
    writerState = 0; // change writer's state to reflect leaving the room
    roomOccupiedbyWriter = 0; // the room is no longer occupied by the writer
    printf("No of people in room %d\n",noInRoom);
    /*
    If another writer is waiting to enter,
    signal the associated condition variable
    */
    if(writerWating == 1){
        //signal that the room is now empty to enter
        err = pthread_cond_signal(&roomEmpty);
        if(err){
            printf("Error signalling condition variable with errno %d\n",err);
            exit(1);
        }
    }
    /*
    If there are no waiting writers,
    broadcast to the waiting readers that the writer is done
    */
    else{
        // broadcast that the writer is done to the waiting readers
        err = pthread_cond_broadcast(&writerDone);
        if(err){
          printf("Error broadcasting condition variable with errno %d\n",err);
          exit(1);
        }
    }
    //unlock the associated mutex
    err = pthread_mutex_unlock(&mutex);
    if(err){
        printf("Error unlocking mutex with errno %d\n",err);
        exit(1);
    }
    return NULL;
}

int main(void) {
    /* You may add code of your own to main() if you want to, but this should not be necessary.
     * Do not change the code that is already here.
     * Note that currently the number of threads is 100 and the writer rate is 1/8.
     * I may try out other values too while marking your assignment.
     */

    int count = 0;
    int seed = time(NULL) % 65536;
    printf("Random seed: %i.\n", seed);
    srand(seed);
    gettimeofday(&time0, NULL);

    pthread_t threads[100];
    for (int i = 0; i < 100; i++) {
        info *info = malloc(sizeof(info));
        if (info == NULL) {
            abort();
        }

        info->id = ++count;
        info->delay = (random() % 6) + (random() % 6) + 2;
        int isWriter = (random() % 8) == 0;
        printf("id=%i isWriter = %i\n", i, isWriter);
        info->thread=&threads[i];
        int err = pthread_create(&threads[i], NULL, isWriter ? writer : reader, info);
        if (err) { abort(); }

        xwait(random() % 5);
    }

    for (int i = 0; i < 100; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err) { abort(); }
    }
}
