/* Server program for key-value store. */
/* compile with gcc *.c -std=gnu99 -o server */

#include "kv.h"
#include "parser.h"
#include "queue.h"

#define NTHREADS 4
#define BACKLOG 10

/* Add anything you want here. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

int data_id[NTHREADS];
pthread_t workers[NTHREADS];

sem_t s_work_avail, s_space_avail, s_shutdown, s_data_lock, s_queue_lock;

Queue q;

/*
* This function is used to initialise the semaphores
* It initialises the data lock, queue lock
* space available, work availabe and shutdown semaphores
* to the appropriate starting values
*/
void initSemaphores(){
    int err;
    //Intialise the semaphores: sem_t s_work_avail, s_space_avail, s_shutdown, s_data_lock, s_queue_lock
    err = sem_init(&s_data_lock, 0, NTHREADS);
    if(err<0){
        printf("Error initialising semaphore\n");
        exit(1);
    }
    err = sem_init(&s_queue_lock, 0, 1);
    if(err<0){
        printf("Error initialising semaphore\n");
        exit(1);
    }
    err = sem_init(&s_space_avail, 0, QUEUE_SIZE);
    if(err<0){
        printf("Error initialising semaphore\n");
        exit(1);
    }
    err = sem_init(&s_work_avail, 0, 0);
    if(err<0){
        printf("Error initialising semaphore\n");
        exit(1);
    }
    err = sem_init(&s_shutdown, 0, 0);
    if(err<0){
        printf("Error initialising semaphore\n");
        exit(1);
    }
    printf("Semaphores initialised\n");
}

/*
* This function is used to destroy the semaphores
* It destroys the semaphores at the end of the program
*/
void destroySemaphores(){
    int err;
    err = sem_destroy(&s_data_lock);
    if(err<0){
        printf("Error destroying semaphore\n");
        exit(1);
    }
    err = sem_destroy(&s_queue_lock);
    if(err<0){
        printf("Error destroying semaphore\n");
        exit(1);
    }
    err = sem_destroy(&s_space_avail);
    if(err<0){
        printf("Error destroying semaphore\n");
        exit(1);
    }
    err = sem_destroy(&s_work_avail);
    if(err<0){
        printf("Error destroying semaphore\n");
        exit(1);
    }
    err = sem_destroy(&s_shutdown);
    if(err<0){
        printf("Error destroying semaphore\n"); 
        exit(1);
    }
    printf("Semaphores destroyed\n");
} 

/*
* This function is used to initialise the sockets
* It initialises sockets for the control and data ports
* It also listens to for request on the socket
*/
int initSocket(int port, struct sockaddr_in s, socklen_t len, int backlog){
    int sockfd,err;
    len = sizeof(s);
    memset(&s, 0, len);
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        printf("Error creating socket");
        exit(1);
    }
    s.sin_family = AF_INET;
    s.sin_addr.s_addr =  htonl(INADDR_ANY);
    s.sin_port = htons(port);
    err = bind(sockfd,(struct sockaddr *)&s, len);
    if(err<0){
        printf("Error binding port %d\n",port);
        exit(1);
    }
    err = listen(sockfd,backlog);
    if(err<0){
        printf("Error listening to port %d\n",port);
        exit(1);
    }
    printf("Initialised socket %d\n",sockfd);
    return sockfd;
}

/*
* This function is used to handle incoming request
* from the data port, parses the command
* and sends appropriate result to client
*/
void handle_data(int conn, char *buffer){
    enum DATA_CMD cmd;
    char *key, *text, *k;
    int r=1, err, n;
    while(r){
        // The client can now enter a command into the terminal
        strncpy(buffer, "\nPlease enter a command > ", LINE);
        write(conn,buffer,LINE);
        // read the command from the client to the buffer and add sentinal value to signal end of line
        int l = read(conn, buffer, LINE);
        buffer[l] = '\0';
        // use the parse function to parse the buffer into commands, key and value
        parse_d(buffer,&cmd,&key,&text);
        //printf("cmd: %i, key = [%s]\n", cmd, key);
        // now handle the requests
        // get a value from the user and return the key if it exists
        if (cmd == D_GET){
            // find the value of the key using findValue
            k = findValue(key);
            // if the value does exist
            if (k != NULL) {
                strncpy(buffer,k,LINE);
            }
            else {
                strncpy(buffer, "No such item.", LINE);
            }
        }
        // check if the client hits return to end connection 
        else if (cmd == D_END) {
            // Copy terminating connection message to buffer
            // exit the loop and close connection
            strncpy(buffer,"Goodbye\n",LINE);
            close(conn);
            r = 0;
        }
        // count the number of items in the store
        else if(cmd == D_COUNT){
            // get the number number of items in the store using countItems
            // convert the number to a character
            n = countItems();
            char l = n + '0';
            strncpy(buffer,&l,LINE);
        }
        // delete an item using its key
        else if(cmd == D_DELETE){
            // use the deleteItem function to delete the key and its value
            // send the appropriate message according to its result
            n = deleteItem(key,1);
            if(n == 0){
                strncpy(buffer, "Delete successful",LINE);
            }else{
                strncpy(buffer, "Deletion error occured",LINE);
            }
        }
        // check if a key exists 
        else if(cmd == D_EXISTS){
            n = itemExists(key);
            if(n>0){
                strncpy(buffer, "Item exists",LINE);
            }
            else{
                strncpy(buffer, "Item doesn't exist",LINE);
            }
        }
        // put a key and its value into the store
        else if(cmd == D_PUT){
            // created an allocated memory for storing values
            char *valCopy = malloc(strlen(text)+1);
            if(valCopy == NULL){
                printf("Error mallocing resource\n");
                exit(1);
            }
            // copy the value into the allocated memory 
            strncpy(valCopy,text,strlen(text)+1);
            // use the createItem function to create this item
            n = createItem(key,valCopy);
            // check if the item has been created from the returned value
            if(n == 0){
                strncpy(buffer, "Item succesfully created",LINE);
            }
            // check for other occurrences and errors
            else if(n<0){
                // if the item already exist update it
                if(itemExists(key)>0){
                    // use updateItem to update the value of the required key
                    n = updateItem(key,valCopy);
                    if(n<0){
                        strncpy(buffer,"Error updating item",LINE);
                    }
                    else{
                        strncpy(buffer,"Key sucsessfully updated",LINE);
                    }
                }
                else{
                    strncpy(buffer,"Error creating item",LINE);
                }
            }
        }
        // check if the line is too long
        else if(cmd == D_ERR_OL){
            strncpy(buffer,"Error, line is too long",LINE);
        }
        // check if the command is invalid
        else if(cmd == D_ERR_INVALID){
            strncpy(buffer,"Error, invalid command: use get, put, count, exists",LINE);
        }
        // check if the parameters exceed required
        else if(cmd == D_ERR_LONG){
            strncpy(buffer,"Error, too many parameters",LINE);
        }
        // check if parameters aren't enough
        else if(cmd == D_ERR_SHORT){
            strncpy(buffer,"Error, too few parameters",LINE);
        }
        else{
            strncpy(buffer,"Please try again\n",LINE);
        }
        //write reponse to user
        write(conn, buffer, LINE);
    }
}

/*
* This function handles worker threads
* The pop connections from the queue stack
* parse the client request from the data port
* using the handle data function
*/
void *worker(void *p){
    int *data = (int *) p;
    char buffer[256];
    int err;
    printf("Worker %u starting.\n", *data);
    int shutdown;
    while (1) {
        // at the start, worker threads wait
        // wait till work is available
        err = sem_wait(&s_work_avail);
        if(err<0){
            exit(1);
        }
        // check if the server is shutting down
        // get the value of s_shutdown semaphore and parse into shutdown variable
        err = sem_getvalue(&s_shutdown, &shutdown);
        if(err<0){
            exit(1);
        }
        // if the server is ready to shutdown break out of the while loop
        if(shutdown == 1){
            break;
        }
        err = sem_wait(&s_queue_lock);
        if(err<0){
            printf("Error waiting on semaphore\n");
            exit(1);
        }
        /* We are now sure there really is work available. */
        // pop the first request on the queue
        int conn = pop(&q);
        // once the request has been popped post that space is now available
        err = sem_post(&s_space_avail);
        if(err<0){
            printf("Error posting semaphore\n");
            exit(1);
        }
        // release lock - sempahore function
        err = sem_post(&s_queue_lock);
        if(err<0){
            printf("Error posting semaphore\n");
            exit(1);
        }
        strncpy(buffer, "Welcome to the KV store.\n", LINE);
        write(conn,buffer,LINE);
        // now handle the commands recieved from client
        // lock the data lock
        err = sem_wait(&s_data_lock);
        if(err){
            printf("Error waiting on semaphore\n");
            exit(1);
        }
        handle_data(conn,buffer);
        err = sem_post(&s_data_lock);
        if(err<0){
            printf("Error posting semaphore\n");
            exit(1);
        } 
    }
    printf("Worker %u shutting down.\n", *data);
    return NULL;
}

/*
* This function handles incoming request 
* from the control port, it parses the data
* and sends appropriate response to the client
*/
int control_data(int conn){
    // create a buffer to read and send data and initialize it
    char buffer[256],letter;
    int count;
    bzero(buffer,256);
    int n = read(conn,buffer,LINE);
    if(n<0){
        printf("Reading error\n");
        exit(1);
    }
    enum CONTROL_CMD cmd;
    // parse the command into the parse functon 
    // for the control port
    cmd = parse_c(buffer);
    // count the number of items in the kv store
    if(cmd == C_COUNT){
        // use countItems count the number of items
        count = countItems();
        sprintf(buffer,"%d  \n",count);
        write(conn,buffer,LINE);
        close(conn);
        return 1;
    }
    // check in case the command isn't recognised
    else if(cmd == C_ERROR){
        strncpy(buffer,"Error\n",LINE);
        write(conn,buffer,LINE);
        close(conn);
        return 1;
    }
    // if the command is to shutdown
    else if(cmd == C_SHUTDOWN){
        strncpy(buffer,"Shutting down\n",LINE);
        write(conn,buffer,LINE);
        close(conn);
        // return zero to break out of the while loop
        // and shutdown the server
        return 0;
    } 
}

//Telnet ends with 2 EOL characters where as terminal only sends 1
//Parser expects a new line at end!

/* You may add code to the main() function. */
int main(int argc, char **argv){
    int cport, dport;		/* control and data ports. */
    struct pollfd fds[2];
    int err, run;
    char buffer[256];
    if (argc < 3) {
	printf("Usage: %s control-port data-port\n", argv[0]);
	exit(1);
    } else {
	cport = atoi(argv[2]);
	dport = atoi(argv[1]);
    }
    // initialise the queue and the semaphores
    initQueue(&q);
    initSemaphores();
    // initialise the sockets to appropriate ports
    int sockfd,fd;
    struct sockaddr_in sA,sB;
    socklen_t lenA,lenB;
    sockfd = initSocket(cport,sA,lenA,1);
    fd = initSocket(dport,sB,lenB,BACKLOG);

    //Create NTHREADS worker threads
    for(int i=0; i<NTHREADS; i++){
        data_id[i] = i;
        err = pthread_create(&workers[i], NULL, worker, &data_id[i]);
        if(err<0){
            exit(1);
        }
    }
    puts("Server started.");
    // add the socket file descriptors to the poll struct
    int timeout = -1;
    int connA,connB;
    fds[0].fd = sockfd;
    fds[1].fd = fd;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    run = 1;
    while(run){
        err = poll(fds,2,timeout);
        if(err<0){
            exit(1);
        }
        else if(err == 0){
            printf("Timeout\n");
        }
        else{
            if(fds[0].revents & POLLIN){
                // handle control request
                connA = accept(sockfd,(struct sockaddr*)&sA, &lenA);
                if(connA<0){
                    printf("Error accepting connection error from control port %d\n",cport);
                    exit(1);
                }
                else{
                    printf("Client[%d] control-port Connect Server OK.\n",cport);
                }
                run = control_data(connA);
            }
            else if(fds[1].revents & POLLIN){
                // handle data request
                // wait until space is available
                // and lock the queue lock
                err = sem_wait(&s_space_avail);
                if(err<0){
                    printf("Error waiting on semaphore\n");
                    exit(1);
                }
                err = sem_wait(&s_queue_lock);
                if(err<0){
                    printf("Error waiting on semaphore\n");
                    exit(1);
                }
                // accept a connection from the client
                connB = accept(fd,(struct sockaddr*)&sB, &lenB);
                if(connB<0){
                    printf("Error accepting connection error from data port %d\n",dport);
                    exit(1);
                }
                else{
                    printf("Client[%d] data-port Connect Server OK.\n",dport);
                }
                // push the accepted connection unto the queue for the worker threads
                push(&q,connB);
                // post work is available for the worker threads 
                err = sem_post(&s_work_avail);
                if(err<0){
                    printf("Error posting semaphore\n");
                    exit(1);
                }
                /* end critical section */
                //Release lock - sempahore function
                err = sem_post(&s_queue_lock);
                if(err<0){
                    printf("Error posting semaphore\n");
                    exit(1);
                }
                printf("Just pushed %d on queue\n", accept);
            }
        }
    }
    close(sockfd);
    close(fd);

    // post the shutdown sempahore for the worker threads to shutdwon
    err = sem_post(&s_shutdown);
    if(err<0){
        printf("Error posting semaphore\n");
        exit(1);
    }
    // its safe to post the work available semaphore as the worker threads
    // will be waiting for this to shutdown
    for(int i=0; i<NTHREADS; i++){
        err = sem_post(&s_work_avail);
        if(err<0){
            printf("Error posting semaphore\n");
            exit(1);
        }
    }
    // join all the worker threads
    for(int i = 0; i<NTHREADS; i++) {
        err = pthread_join(workers[i], NULL);
         if(err){
            printf("Error joining threads\n");
            exit(1);
         }
    }
    // function destroys all the semaphores
    destroySemaphores();

    return 0;
}
