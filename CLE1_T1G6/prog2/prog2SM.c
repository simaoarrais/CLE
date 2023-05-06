/**
 * @file prog1SM.c (implementation file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Bitonic Integer Sorting.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the threads:
 *     \li (distributor) readFromFileAndStore
 *     \li (distributor) distributeRanges
 *     \li (main) storeFileName
 *     \li (main) validateSequence
 *     \li (worker) fetchSubSequence
 *     \li (worker) signalFinished.
 *
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#include "probConst.h"
#include "prog2SM.h"

/** \brief return status on monitor initialization */
extern int statusInitMon;

/** \brief distributor thread return status */
extern int statusDistributor;

/** \brief worker threads return status array */
extern int *statusWorker;

/** \brief main/generator thread return status */
extern int statusMain;

/** \brief number of threads input by the user */
extern int nThreads;

/** \brief sorting order, positive integer for increasing */
extern int dir;

// Shared memory
/** \brief file name for file storing the sequence */
static char * file;

/** \brief the sequence as an array of integers, initially unordered */
static int * sequence;

/** \brief 2D array indicating the sequence range assigned to each worker */
static unsigned int ** workerRange;

/** \brief array of status attributed to each worker */
static int * workerCommand;

/** \brief size of the sequence */
static int sequenceSize;

/** \brief number of workers waiting for work */
static int waitingWorkers;

/** \brief number of workers finished sorting their sequence */
static int finishedWorkers;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief flag which warrants that the data transfer region is initialized exactly once */
static pthread_once_t init = PTHREAD_ONCE_INIT;

/** \brief distributor synchronization point when all workers are finished sorting */
static pthread_cond_t allWorkersFinished;

/** \brief distributor synchronization point when all workers are waiting for work */
static pthread_cond_t allWorkersWaiting;

/** \brief worker synchronization point when new work run starts */
static pthread_cond_t waitForWork;

/**
 *  \brief Initialization of the data transfer region.
 *
 *  Internal monitor operation.
 */
static void initialization(void) {
    // Allocate space for the shared memory structures
    if(((file = (char *)malloc((MAXFILENAMELEN+1) * sizeof(char))) == NULL) ||
       ((workerCommand = (int *)malloc(nThreads * sizeof(int))) == NULL) ||
       ((workerRange = (unsigned int **)malloc(nThreads * sizeof(unsigned int *))) == NULL)) {
        fprintf (stderr, "Error on allocating space to the data transfer region!\n");
        statusInitMon = EXIT_FAILURE;
        pthread_exit (&statusInitMon);
    }
    for(int i = 0; i < nThreads; i++) {
        if(((workerRange[i] = (unsigned int *)malloc(2 * sizeof(unsigned int))) == NULL)) {
            fprintf (stderr, "Error on allocating space to the data transfer region!\n");
            statusInitMon = EXIT_FAILURE;
            pthread_exit (&statusInitMon);
        }
    }

    // initialize shared memory structures
    sequenceSize = 0;
    for(int i = 0; i < nThreads; i++) {
        for(int j = 0; j < 2; j++) {
            workerRange[i][j] = 0;
        }
        workerCommand[i] = AVAILABLE; // all workers start available, later altered by the distributor
    }
    waitingWorkers = 0; // no workers waiting or finished initially
    finishedWorkers = 0;

    // conditions initialization
    pthread_cond_init(&allWorkersWaiting, NULL);
    pthread_cond_init(&allWorkersFinished, NULL);
    pthread_cond_init(&waitForWork, NULL);
}

/**
 * @brief Store file name in the data transfer region.
 * 
 * Operation carried out by the main thread after processing user input.
 * 
 * @param fileName name of the file to be stored
 */
void storeFileName(char * fileName) {
    statusMain = pthread_mutex_lock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread entering monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
    pthread_once(&init, initialization);

    file = fileName;

    statusMain = pthread_mutex_unlock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread exiting monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
}

/**
 * @brief Open the file and store the sequence and its size in shared memory.
 * 
 * Operation carried out by the distributor thread on initialization.
 * 
 */
void readFromFileAndStore() {
    statusDistributor = pthread_mutex_lock(&accessCR);
    if(statusDistributor) {
        errno = statusDistributor;
        perror("Error on distributor thread entering monitor (CF).");
        statusDistributor = EXIT_FAILURE;
    }

    FILE * fp = fopen(file, "rb");

    // read sequence size
    fread(&sequenceSize, sizeof(int), 1, fp);

    // allocate space for sequence
    if(((sequence = (int *)malloc(sequenceSize * sizeof(int))) == NULL )) {
        fprintf (stderr, "Error on allocating space to the data transfer region!\n");
        statusInitMon = EXIT_FAILURE;
        pthread_exit (&statusInitMon);
    }

    // store sequence
    int val;
    for(int i = 0; i < sequenceSize; i++) {
        fread(&val, sizeof(int), 1, fp);
        sequence[i] = val;
    }

    fclose(fp);

    statusDistributor = pthread_mutex_unlock(&accessCR);
    if(statusDistributor) {
        errno = statusDistributor;
        perror("Error on distributor thread exiting monitor (CF).");
        statusDistributor = EXIT_FAILURE;
    }
}

/**
 * @brief Distribute sequence ranges and commands to the various workers.
 * 
 * Operation carried out by the distributor thread.
 * 
 * Command and ranges are assigned to each worker, then the thread awaits the work to be finished and reduces de number of active workers in half.
 * 
 * @param activeWorkers number of workers currently still alive
 */
void distributeRanges(int * activeWorkers) {
    statusDistributor = pthread_mutex_lock(&accessCR);
    if(statusDistributor) {
        errno = statusDistributor;
        perror("Error on distributor thread entering monitor (CF).");
        statusDistributor = EXIT_FAILURE;
    }

    // wait for all workers to be free
    while(waitingWorkers < *activeWorkers) {
        if((statusDistributor = pthread_cond_wait(&allWorkersWaiting, &accessCR)) != 0) {
            errno = statusDistributor;
            perror("Error on waiting in allWorkersWaiting");
            statusDistributor = EXIT_FAILURE;
            pthread_exit(&statusDistributor);
        }
    }
    
    // size of sequence chunk to be given to each worker (equal)
    int chunkSize = sequenceSize / *activeWorkers;
    int currSeqPointer = 0;
    for(int i = 0; i < nThreads; i++) { // distribute ranges to the workers
        if(workerCommand[i] == AVAILABLE) { // only alive workers
            workerRange[i][0] = currSeqPointer;
            workerRange[i][1] = currSeqPointer + chunkSize - 1;
            if((*activeWorkers == 1) && (*activeWorkers == nThreads)) { // only one worker assigned to the task, sort the non bitonic sequence in the order specified
                workerCommand[i] = (dir < 0) ? ORDER_NON_BITONIC_DCR : ORDER_NON_BITONIC_INCR;
            }
            else if(*activeWorkers == 1) { // only one worker left, sort the bitonic sequence in the order specified
                workerCommand[i] = (dir < 0) ? ORDER_BITONIC_DCR : ORDER_BITONIC_INCR;
            }
            else if(*activeWorkers == nThreads) { // initial run, order non bitonic sequences (incr if worker id is even)
                workerCommand[i] = (i % 2 == 0) ? ORDER_NON_BITONIC_INCR : ORDER_NON_BITONIC_DCR;
            }
            else workerCommand[i] = (i % 2 == 0) ? ORDER_BITONIC_INCR : ORDER_BITONIC_DCR; // order bitonic sequences (incr if worker id is even)
            currSeqPointer = workerRange[i][1] + 1;
        }
    } 

    // reset finished workers
    finishedWorkers = 0;

    // broadcast workers to work
    if((statusDistributor = pthread_cond_broadcast(&waitForWork)) != 0) {
        errno = statusDistributor;
        perror("Error on broadcasting waitForWork");
        statusDistributor = EXIT_FAILURE;
        pthread_exit(&statusDistributor);
    }

    // wait for workers to finish
    while(finishedWorkers != *activeWorkers) { // while not all workers have finsihed, wait
        if((statusDistributor = pthread_cond_wait(&allWorkersFinished, &accessCR)) != 0) {
            errno = statusDistributor;
            perror("Error on waiting in allWorkersFinished");
            statusDistributor = EXIT_FAILURE;
            pthread_exit(&statusDistributor);
        }
    }

    // reduce number of active workers in half
    *activeWorkers /= 2;

    // signal which worker's die (low-id workers given priority to live on)
    for(int i = 0; i < nThreads; i++) {
        workerCommand[i] = (i < *activeWorkers) ? AVAILABLE : DIE;
    }

    // broadcast workers to move on
    if((statusDistributor = pthread_cond_broadcast(&waitForWork)) != 0) {
        errno = statusDistributor;
        perror("Error on broadcasting waitForWork");
        statusDistributor = EXIT_FAILURE;
        pthread_exit(&statusDistributor);
    }

    statusDistributor = pthread_mutex_unlock(&accessCR);
    if(statusDistributor) {
        errno = statusDistributor;
        perror("Error on distributor thread exiting monitor (CF).");
        statusDistributor = EXIT_FAILURE;
    }
}

/**
 * @brief Fetches the pointer to the sequence to sort, as well as the sorting type and the sequence size.
 * 
 * Operation carried out by worker threads.
 * 
 * @param workerID worker identification
 * @param command output variable, indicates the type of sorting to be carried out
 * @param chunkSize output variable, size of the subsequence to be sorted
 * @param chunk output varibale, pointer to the beginning of the subsequence to be sorted
 * @return true : the worker's work is finished signaling it should quit 
 * @return false : the worker should continue it's life cycle
 */
bool fetchSubSequence(unsigned int workerID, int * command, int * chunkSize, int ** chunk) {
    statusWorker[workerID] = pthread_mutex_lock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on distributor thread entering monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
    }

    // check assigned state, if DIE quit successfully
    if(workerCommand[workerID] == DIE) {
        statusWorker[workerID] = pthread_mutex_unlock(&accessCR); // leave monitor
        if(statusWorker[workerID]) {
            errno = statusWorker[workerID];
            perror("Error on leaving monitor (CF).");
            statusWorker[workerID] = EXIT_FAILURE;
            pthread_exit(&statusWorker[workerID]);
        }
        return true;
    }

    waitingWorkers += 1; // waiting for work

    // signal distributor you are waiting
    if((statusWorker[workerID] = pthread_cond_signal(&allWorkersWaiting)) != 0) {
        errno = statusWorker[workerID];
        perror("Error on signal in allWorkersWaiting");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    // wait for distributor to distribute ranges and commands
    if((statusWorker[workerID] = pthread_cond_wait(&waitForWork, &accessCR)) != 0) {
        errno = statusWorker[workerID];
        perror("Error on waiting in waitForWork");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    waitingWorkers -= 1; // no longer waiting

    // fetch output variables
    *command = workerCommand[workerID];
    *chunkSize = workerRange[workerID][1] - workerRange[workerID][0] + 1;
    *chunk = &sequence[workerRange[workerID][0]];

    statusWorker[workerID] = pthread_mutex_unlock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on distributor thread exiting monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
    }

    return false;
}

/**
 * @brief Signal sorting is finished and was successful.
 * 
 * Operation carried out by the workers after successful sorting of the assigned sequence.
 * 
 * @param workerID worker identification
 */
void signalFinished(unsigned int workerID) {
    statusWorker[workerID] = pthread_mutex_lock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on distributor thread entering monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
    }

    finishedWorkers += 1; // work finished
    workerCommand[workerID] = AVAILABLE; // update status

    // signal distributor you are free
    if((statusWorker[workerID] = pthread_cond_signal(&allWorkersFinished)) != 0) {
        errno = statusWorker[workerID];
        perror("Error on signal in allWorkersFinished");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    // wait for distributor to close up this run
    if((statusWorker[workerID] = pthread_cond_wait(&waitForWork, &accessCR)) != 0) {
        errno = statusWorker[workerID];
        perror("Error on waiting in waitForWork");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    statusWorker[workerID] = pthread_mutex_unlock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on distributor thread exiting monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
    }
}

/**
 * @brief Validate if the sequence was properly sorted.
 * 
 * Operation carried out by main thread after all workers have quit.
 * 
 */
void validateSequence() {
    statusMain = pthread_mutex_lock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread entering monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
    pthread_once(&init, initialization);

    // Validate
    int i; 
    for(i = 0; i < sequenceSize-1; i++) {
        if(dir < 0) {
            if(sequence[i] < sequence[i+1]) {
            printf ("Error in position %d between element %d and %d\n",
                    i, sequence[i], sequence[i+1]);
            break;
            }
        } else {
            if(sequence[i] > sequence[i+1]) {
            printf ("Error in position %d between element %d and %d\n",
                    i, sequence[i], sequence[i+1]);
            break;
            }
        }
    }
    if(i == (sequenceSize-1)) printf("Everything is OK!\n");

    statusMain = pthread_mutex_unlock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread exiting monitor (CF).");
        statusMain = EXIT_FAILURE;
    }    
}
