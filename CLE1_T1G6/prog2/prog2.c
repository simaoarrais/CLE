/**
 * @file prog1.c
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Bitonic Integer Sorting.
 * 
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of Lampson / Redell type.
 *
 *  Generator thread of the intervening entities.
 * 
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "prog2SM.h"
#include "prog2Utils.h"
#include "probConst.h"

/** \brief return status on monitor initialization */
int statusInitMon;

/** \brief distributor thread return status */
int statusDistributor;

/** \brief worker threads return status array */
int *statusWorker;

/** \brief main/generator thread return status */
int statusMain;

/** \brief worker life cycle routine */
static void *worker(void *args);

/** \brief distributor life cycle routine */
static void *distributor(void *args);

/** \brief execution time measurement */
static double get_delta_time(void);

/** \brief number of threads input by the user */
int nThreads = 4;

/** \brief sorting order, positive integer for increasing */
int dir = 1;

/**
 * @brief Main thread.
 *
 *  Its role is starting the simulation by generating the intervening entities threads (workers and distributor) and
 *  waiting for their termination.
 * 
 *  \param argc number of words of the command line
 *  \param argv list of words of the command line
 *
 *  \return status of operation
 */
int main(int argc, char *argv[]) {
    // Process the command line options
    int opt;
    char * file;

    if((file = (char *)malloc((MAXFILENAMELEN+1) * sizeof(char))) == NULL) {
        fprintf(stderr, "Error allocating memory.\n");
        return EXIT_FAILURE;
    }
    
    // TODO: check missing file
    opterr = 0;
    do {
        bool errFlg = false;
        switch (opt = getopt(argc, argv, "t:f:d:")) {
            case 't':
                if(atoi(optarg) <= 0) {
                    fprintf(stderr, "%s: number of threads must be a positive integer!\n", basename(argv[0]));
                    errFlg = true;
                }
                nThreads = (int) atoi(optarg);
                break;
            case 'f':
                if(strlen(optarg) > MAXFILENAMELEN) {
                    fprintf(stderr, "%s: file names may not be larger than %i characters!\n", basename(argv[0]), MAXFILENAMELEN);
                    errFlg = true;
                }
                file = optarg;
                break;
            case 'd':
                if(atoi(optarg) == 0) {
                    fprintf(stderr, "%s: direction must be an integer different from 0 (positive -> crescent order).\n", basename(argv[0]));
                    errFlg = true;
                }
                dir = (int) atoi(optarg);
                break;
            case '?': 
                fprintf (stderr, "%s: invalid option\n", basename (argv[0]));
                errFlg = true;
            case -1: break;
            // TODO: print usage
        }
        if(errFlg) return EXIT_FAILURE;
    } while(opt != -1);
    if(argc == 1) {
        fprintf (stderr, "%s: invalid format\n", basename (argv[0]));
        return EXIT_FAILURE;
    }

    if((statusWorker = malloc (nThreads * sizeof (int))) == NULL) {
        fprintf(stderr, "Error on allocating space to the return status arrays of worker threads.\n");
        exit(EXIT_FAILURE);
    }

    pthread_t *tIdWorkers;
    pthread_t tIdDistributor;
    unsigned int *workers;
    int *pStatus;

    /* initializing the application defined thread id arrays for the workers and distributor and the random number
        generator */

    if (((tIdWorkers = malloc(nThreads * sizeof (pthread_t))) == NULL) ||
        ((workers = malloc(nThreads * sizeof (unsigned int))) == NULL)) {
        fprintf(stderr, "error on allocating space to both internal / external worker id arrays\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nThreads; i++) workers[i] = i;

    srandom ((unsigned int) getpid());
    (void) get_delta_time();

    // store name of file in SM
    storeFileName(file);

    // create distributor and worker threads and wait for termination
    if(pthread_create(&tIdDistributor, NULL, distributor, NULL) != 0) {
        perror("Error on creating thread distributor.");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nThreads; i++)
    if(pthread_create (&tIdWorkers[i], NULL, worker, &workers[i]) != 0) { 
        perror("Error on creating thread worker.");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nThreads; i++) { 
        if (pthread_join(tIdWorkers[i], (void *) &pStatus) != 0) {
            perror("error on waiting for thread worker");
            exit(EXIT_FAILURE);
        }
        printf("Thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *pStatus);
    }
    if(pthread_join(tIdDistributor, (void *) &pStatus) != 0) {
        perror("error on waiting for thread distributor");
        exit(EXIT_FAILURE);
    }
    printf("Thread distributor has terminated:");
    printf("its status was %d\n", *pStatus);

    // check if sequence is properly sorted
    validateSequence();

    printf ("\nElapsed time = %.6f s\n", get_delta_time ());
    
    return 0;
}

/**
 * @brief Distributor function.
 * 
 * Its role is to simulate the life cycle of the distributor.
 * 
 * @param args
 * @return void* 
 */
static void *distributor(void * args) {
    // read file sequence and store in SM
    readFromFileAndStore();
    int nActiveWorkers = nThreads;

    while(true) { // while the whole sequence is not sorted
        // distribute ranges to workers (reduce worker number in half each iteration)
        distributeRanges(&nActiveWorkers);
        if(nActiveWorkers == 0) break;
    }
    
    statusDistributor = EXIT_SUCCESS;
    pthread_exit(&statusDistributor);
}

/**
 * @brief Worker funtion.
 * 
 * Its role is to simulate the life cycle of a worker.
 * 
 * @param args pointer to application defined worker identification
 * @return void* 
 */
static void *worker(void * args) {
    unsigned int id = *((unsigned int *) args);
    bool quit = false;

    while(true) {
        int command = 0;
        int chunkSize;
        int * chunk;
        // get pointer to subsequence and its size
        quit = fetchSubSequence(id, &command, &chunkSize, &chunk);

        if(quit) break;

        // sort the subsequence
        int localDir = command < 0 ? -1 : 1;
        if(command == ORDER_NON_BITONIC_DCR || command == ORDER_NON_BITONIC_INCR) bitonicSort(&chunk, 0, chunkSize, localDir);
        else if(command == ORDER_BITONIC_DCR || command == ORDER_BITONIC_INCR) bitonicMerge(&chunk, 0, chunkSize, localDir);

        // tell distributor you're finished sorting
        signalFinished(id);
    }

    statusWorker[id] = EXIT_SUCCESS;
    pthread_exit(&statusWorker[id]);
}

/**
 *  \brief Get the process time that has elapsed since last call of this time.
 *
 *  \return process elapsed time
 */

static double get_delta_time(void) {
    static struct timespec t0, t1;

    t0 = t1;
    if(clock_gettime (CLOCK_MONOTONIC, &t1) != 0) {
        perror ("clock_gettime");
        exit(1);
    }
    return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}
