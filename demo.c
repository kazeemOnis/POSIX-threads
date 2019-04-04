#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/*
 * Parameters of the program. The constraints are D < T and
 * S*K <= M*N.
 */
struct demo_parameters {
    int S;   /* Number of students */
    int M;   /* Number of markers */
    int K;   /* Number of markers per demo */
    int N;   /* Number of demos per marker */
    int T;   /* Length of session (minutes) */
    int D;   /* Length of demo (minutes) */
};

/* Global object holding the demo parameters. */
struct demo_parameters parameters;

/* The demo start time, set in the main function. Do not modify this. */
struct timeval starttime;

/*
 * You may wish to place some global variables here.
 * Remember, globals are shared between threads.
 * You can also create functions of your own.
 */

// mutex used for controlling permission to variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// condition to be signalled when a marker has been grabbed
pthread_cond_t grabCond  = PTHREAD_COND_INITIALIZER;
// condtion to be signalled when the demo is about to start
pthread_cond_t demoStart = PTHREAD_COND_INITIALIZER;
// condition to be signalled when a demo is completed
pthread_cond_t demoEnd = PTHREAD_COND_INITIALIZER;
// condition to be signalled when markers are available
pthread_cond_t markersAvail = PTHREAD_COND_INITIALIZER;

// keep track of the number of available markers
int availMarkers;
// keep track of whether there is enough time to run a demo
// 0 = time available, 1 = time is up
int timesUp;

// struct data used to represent a student's data
typedef struct studData{
    // keep track of the demo started state, 0 = started, 1 = not started
    int demoStarted;
    // keep track of the the demo finished state, 0 = not finished, 1 = finished;
    int demoFinished;
} StudData;
// global array for storing students data
StudData *studArray;

// struct data used to represent a marker's data
typedef struct markerData{
    // each marker has a state, 0 = idle, 1 = grabbed, -1 = exit lab
    int markerState;
    // used to represent the student that has grabbed marker, -1 default
    int studId;
} MarkData;
// global array for storing markers data
MarkData *markArray;

/* createData(): used to intialise the global variables, students and markers data arrays*/
void createData(){
    // set availMarkers to M at the start of the session
    availMarkers = parameters.M;
    // set time's up to time available at the start of the session
    timesUp = 0;
    for(int i=0; i<parameters.S; i++){
        // set all the students to a demo state of not started and not finished
        studArray[i].demoStarted = 0;
         studArray[i].demoFinished = 0;
    }
    for(int j=0; j<parameters.M; j++){
        // set all the markers to an idle state
        markArray[j].markerState = 0;
        // set the id of the markers student id to default
        markArray[j].studId = -1;
    }
}

/*
 * timenow(): returns current simulated time in "minutes" (cs).
 * Assumes that starttime has been set already.
 * This function is safe to call in the student and marker threads as
 * starttime is set in the run() function.
 */
int timenow() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - starttime.tv_sec) * 100 + (now.tv_usec - starttime.tv_usec) / 10000;
}

/* delay(t): delays for t "minutes" (cs) */
void delay(int t) {
    struct timespec rqtp, rmtp;
    t *= 10;
    rqtp.tv_sec = t / 1000;
    rqtp.tv_nsec = 1000000 * (t % 1000);
    nanosleep(&rqtp, &rmtp);
}

/* panic(): simulates a student's panicking activity */
void panic() {
    delay(random() % (parameters.T - parameters.D));
}

/* demo(): simulates a demo activity */
void demo() {
    delay(parameters.D);
}

/*
 * A marker thread. You need to modify this function.
 * The parameter arg is the number of the current marker and the function
 * doesn't need to return any values.
 * Do not modify the printed output as it will be used as part of the testing.
 */
void *marker(void *arg) {
    int markerID = *(int *)arg;
    int err; // error handling variable
    /*
     * The following variable is used in the printf statements when a marker is
     * grabbed by a student. It shall be set by this function whenever the
     * marker is grabbed - and before the printf statements referencing it are
     * executed.
     */
    int studentID;
    /*
     * The following variable shall indicate which job the marker is currently
     * executing, the first being job 0. The variable ranges from 0 to
     * (parameters.N - 1) .
     */
    int job = 0;

    /* 1. Enter the lab. */
    printf("%d marker %d: enters lab\n", timenow(), markerID);

    /* A marker marks up to N projects. */
    /* 2. Repeat (N times).
     *    (a) Wait to be grabbed by a student.
     *    (b) Wait for the student's demo to begin
     *        (you may not need to do anything here).
     *    (c) Wait for the demo to finish.
     *        Do not just wait a given time -
     *        let the student signal when the demo is over.
     *    (d) Exit the lab.
     */
     for(int i=0; i<parameters.N; i++){
        // lock the associated mutex
        err = pthread_mutex_lock(&mutex);
        if(err){
            exit(1);
        }
        // wait until the marker is in a grabbed state and times available
        while((markArray[markerID].markerState != 1) && !(timesUp)){
            // wait to recieve the grabCond condition variable
            err = pthread_cond_wait(&grabCond,&mutex);
            if(err){
                exit(1);
            }
        }
        // if there is no available time to complete a demo
        if(timesUp){
            // unlock the associated mutex to avoid deadlock
            err = pthread_mutex_unlock(&mutex);
            if(err){
                exit(1);
            }
            // exit the lab with  timeout
            printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
            return NULL;

        }
        // set the studentId to indicate the ID of the student who grabbed the marker
        studentID = markArray[markerID].studId;
        // unlock the associated mutex
        err = pthread_mutex_unlock(&mutex);
        if(err){
            exit(1);
        }
        // check if the marker has now been grabbed
        printf("%d marker %d: grabbed by student %d (job %d)\n", timenow(), markerID, studentID, job + 1);
        // lock the associated mutex
        err = pthread_mutex_lock(&mutex);
        if(err){
            exit(1);
        }
        // wait for the demo start
        while(studArray[studentID].demoStarted != 1){
        // wait to recieve the demoStart condition variable
        err = pthread_cond_wait(&demoStart,&mutex);
        if(err){
                exit(1);
            }
        }
        // once the marker reaches this stage, they begin the demo
        if(studArray[studentID].demoStarted == 1){
            // then proceed to waiting for the demo to finsish
            while(studArray[studentID].demoFinished != 1){
                // wait to recieve the demo end condition variable
                err = pthread_cond_wait(&demoEnd,&mutex);
                if(err){
                    exit(1);
                }
            }
            // unlock the associated mutex
            err = pthread_mutex_unlock(&mutex);
            if(err){
                exit(1);
            }
            printf("%d marker %d: finished with student %d (job %d)\n", timenow(), markerID, studentID, job + 1);
            // increment the number of jobs completed
            job++;
            err = pthread_mutex_lock(&mutex);
            if(err){
                exit(1);
            }
            // check whether the amount of jobs completed is equal to N
            if(job<parameters.N){
                // if marker hasn't completed N demos re-enter lab
                // increment the markers and set markers state to idle
                availMarkers++;
                markArray[markerID].markerState = 0;
                // broadcast markers avail to any waiting students
                err = pthread_cond_broadcast(&markersAvail);
            }
            else{
                // otherwise set the marker state to exit the lab
                markArray[markerID].markerState = -1;
            }
            // unlock the associated mutex
            err = pthread_mutex_unlock(&mutex);
            if(err){
                exit(1);
            }
        }
     }
     /*
     * 3. If the end of the session approaches (i.e. there is no time
     *    to start another demo) then the marker waits for the current
     *    demo to finish (if they are currently attending one) and then
     *    exits the lab.
     */
     if(job != parameters.N){
        // lock the associated mutex
        err = pthread_mutex_lock(&mutex);
        if(err){
            exit(1);
        }
        // check if the marker is currently occupied
        while(markArray[markerID].markerState == 1){
            // wait for the demo to end condtion variable
            err = pthread_cond_wait(&demoEnd,&mutex);
            if(err){
                exit(1);
            }
        }
        // unlock the associated mutex
        err = pthread_mutex_unlock(&mutex);
        if(err){
            exit(1);
        }
     }
     // once the marker has completed N demos
     else if((job) == parameters.N){
        // exit the lab appropriately
        printf("%d marker %d: exits lab (finished %d jobs)\n", timenow(), markerID, parameters.N);
     }
    return NULL;
}


/*
 * A student thread. You must modify this function.
 */
void *student(void *arg) {
    /* The ID of the current student. */
    int studentID = *(int *)arg;
    int err; // error handling variable
    int count = 0; // counter variable

    /* 1. Panic! */
    printf("%d student %d: starts panicking\n", timenow(), studentID);
    panic();

    /* 2. Enter the lab. */
    printf("%d student %d: enters lab\n", timenow(), studentID);

    /* 3. Grab K markers. */
    // lock the associated mutex
    err = pthread_mutex_lock(&mutex);
    if(err){
        exit(1);
    }
    // check if the available markers are enough to proceed otherwise wait
    // also check if there is enough time to proceed
    while((availMarkers < parameters.K) && !(timesUp)){
        err = pthread_cond_wait(&markersAvail,&mutex);
        if(err){
            exit(1);
        }
    }
    // if timesUP release mutex and print time's Up message
    if(timesUp){
        err = pthread_mutex_unlock(&mutex);
        if(err){
            exit(1);
        }
        printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
        return NULL;
    }

    // test for the satisfied condition variable
    // i.e if there enough  available Markers
    if(availMarkers >= parameters.K){
        // proceed to grabbing K markers
        for(int i=0; i<parameters.K; i++){
            // search for idle markers to grab
            for(int j=0; j<parameters.M; j++){
                if(markArray[j].markerState == 0){
                    // set the selected marker to a grabbed state
                    markArray[j].markerState = 1;
                    // set the studentId of the grabbed marker to this student
                    markArray[j].studId = studentID;
                    // decrease the number of available markers since they've been grabbed
                    availMarkers--;
                    // increment the count variable to keep count of the markers grabbed
                    count++;
                    // set the demo start and finish states
                    studArray[studentID].demoStarted  = 0;
                    studArray[studentID].demoFinished  = 0;
                    // signal to the idle marker waiting to be grabbed and wake it up
                    if(markArray[j].markerState == 1){
                        err = pthread_cond_broadcast(&grabCond);
                        if(err){
                            exit(1);
                        }
                    }
                    break;
                }
            }
        }
        // unlock the associated mutex as you are about to break out of the for loop
        err = pthread_mutex_unlock(&mutex);
        if(err){
            exit(1);
        }
        /* 4. Demo! */
        /*
         * If the student succeeds in grabbing K markers and there is enough time left
         * for a demo, the following three lines shall be executed in order.
         * If the student has not started their demo and there is not sufficient time
         * left to do a full demo, the following three lines shall not be executed
         * and the student proceeds to step 5.
         */
        //check if there are enough markers to complete a demo
        if(count == parameters.K){
            // lock the associated mutex
            err = pthread_mutex_lock(&mutex);
            if(err){
                exit(1);
            }
            // set the demo start variable to 1 = started
            studArray[studentID].demoStarted = 1;
            // broadcast to the waiting marker threads that the demo is about to start
            err = pthread_cond_broadcast(&demoStart);
            if(err){
                exit(1);
            }
            // unlock the appropriated mutex to print
            err = pthread_mutex_unlock(&mutex);
            if(err){
                exit(1);
            }
            // proceed to starting and ending the demo
            printf("%d student %d: starts demo\n", timenow(), studentID);
            demo();
            printf("%d student %d: ends demo\n", timenow(), studentID);
            // relock the associated mutex after printing
            err = pthread_mutex_lock(&mutex);
            if(err){
                exit(1);
            }
            // set the demo finish variable to = 1 finsihed
            studArray[studentID].demoFinished = 1;
            // broadcast to waiting marker threads that the demo has finished
            err = pthread_cond_broadcast(&demoEnd);
            if(err){
                exit(1);
            }
            // unlock the associated mutex and exit the lab
            err = pthread_mutex_unlock(&mutex);
            if(err){
                exit(1);
            }
            printf("%d student %d: exits lab (finished)\n", timenow(), studentID);
        }
    }
    return NULL;
}

/* The function that runs the session. */
void run() {
    int i;
    int ok;
    int markerID[100], studentID[100];
    pthread_t markerT[100], studentT[100];
    // allocate memory for student and marker array data
    studArray = (StudData *)malloc(parameters.S*sizeof(StudData));
    markArray = (MarkData *)malloc(parameters.M*sizeof(MarkData));

    // error handling
    if(markArray == NULL || studArray == NULL){ /* Test to see if the allocation of memory failed */
        puts("Out of memory!");
        exit(1);
    }
    createData(); /* initialise the markers and students data */
    printf("S=%d M=%d K=%d N=%d T=%d D=%d\n",
        parameters.S,
        parameters.M,
        parameters.K,
        parameters.N,
        parameters.T,
        parameters.D);
    gettimeofday(&starttime, NULL);  /* Save start of simulated time */

    /* Create S student threads */
    for (i = 0; i<parameters.S; i++) {
        studentID[i] = i;
        ok = pthread_create(&studentT[i], NULL, student, &studentID[i]);
         if (ok != 0) { abort(); }
    }
    /* Create M marker threads */
    for (i = 0; i<parameters.M; i++) {
        markerID[i] = i;
        ok = pthread_create(&markerT[i], NULL, marker, &markerID[i]);
         if (ok != 0) { abort(); }
    }

    /* With the threads now started, the session is in full swing ... */
    delay(parameters.T - parameters.D + 1);

    /*
     * When we reach here, this is the latest time a new demo could start.
     * You might want to do something here or soon after.
     */
     int err;
     // lock the associated mutex
     err = pthread_mutex_lock(&mutex);
     if(err){
        exit(1);
     }
     // since there is not enough time to start a demo set times up
     timesUp = 1;
     // broadcast to any waiting markers to exit with timeout
     err = pthread_cond_broadcast(&grabCond);
     if(err){
        exit(1);
     }
     // broadcast to any waiting students to exit with timeout
     err = pthread_cond_broadcast(&markersAvail);
     if(err){
        exit(1);
     }
     // unlock the associated mutex
     err = pthread_mutex_unlock(&mutex);
     if(err){
        exit(1);
     }
    /* Wait for student threads to finish */
    for (i = 0; i<parameters.S; i++) {
        ok = pthread_join(studentT[i], NULL);
         if (ok != 0) { abort(); }
    }

    /* Wait for marker threads to finish */
    for (i = 0; i<parameters.M; i++) {
        ok = pthread_join(markerT[i], NULL);
         if (ok != 0) { abort(); }
    }

    /* Free the resources that have been created for memory allocation */
    free(studArray);
    free(markArray);
}

/*
 * main() checks that the parameters are ok. If they are, the interesting bit
 * is in run() so please don't modify main().
 */
int main(int argc, char *argv[]) {
    if (argc < 6) {
        puts("Usage: demo S M K N T D\n");
        exit(1);
    }
    parameters.S = atoi(argv[1]);
    parameters.M = atoi(argv[2]);
    parameters.K = atoi(argv[3]);
    parameters.N = atoi(argv[4]);
    parameters.T = atoi(argv[5]);
    parameters.D = atoi(argv[6]);
    if (parameters.M > 100 || parameters.S > 100) {
        puts("Maximum 100 markers and 100 students allowed.\n");
        exit(1);
    }
    if (parameters.D >= parameters.T) {
        puts("Constraint D < T violated.\n");
        exit(1);
    }
    if (parameters.S*parameters.K > parameters.M*parameters.N) {
        puts("Constraint S*K <= M*N violated.\n");
        exit(1);
    }

    srand(time(NULL));

    // We're good to go.

    run();


    return 0;
}
