/**
 * @file prog1.c
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Vowel Count.
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

#include "prog1SM.h"
#include "probConst.h"
#include "prog1Utils.h"

/** \brief list of characters considered vowels */
static unsigned char vowels[6] = {'a', 'e', 'i', 'o', 'u', 'y'};

/** \brief return status on monitor initialization */
int statusInitMon;

/** \brief worker threads return status array */
int *statusWorker;

/** \brief main/generator thread return status */
int statusMain;

/** \brief worker life cycle routine */
static void *worker(void *args);

/** \brief execution time measurement */
static double get_delta_time(void);

/** \brief number of files input by the user */
int nFiles;

/** \brief number of threads input by the user */
int nThreads = 4;

/**
 * @brief Main thread.
 *
 *  Its role is starting the simulation by generating the intervening entities threads (workers) and
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
    char ** files;

    nFiles = 0;
    if((files = (char **)malloc(MAXFILECOUNT * sizeof(char *))) == NULL) {
        fprintf(stderr, "Error allocating memory.\n");
        return EXIT_FAILURE;
    }
    for(int i = 0; i < MAXFILECOUNT; i++) {
        if((files[i] = (char *)malloc((MAXFILENAMELEN+1) * sizeof(char))) == NULL) {
            fprintf(stderr, "Error allocating memory.\n");
            return EXIT_FAILURE;
        }
    }
    
    opterr = 0;
    do {
        bool errFlg = false;
        switch (opt = getopt(argc, argv, "t:f:")) {
            case 't':
                if(atoi(optarg) <= 0) {
                    fprintf(stderr, "%s: number of threads must be a positive integer!\n", basename(argv[0]));
                    errFlg = true;
                }
                nThreads = (int) atoi(optarg);
                break;
            case 'f':
                optind--;
                for(;optind < argc && *argv[optind] != '-'; optind++) {
                    if(nFiles >= MAXFILECOUNT) {
                        fprintf(stderr, "%s: you may only select up to %i files!\n", basename(argv[0]), MAXFILECOUNT);
                        errFlg = true;
                        break;
                    }
                    char* fileName = argv[optind];
                    if(strlen(fileName) > MAXFILENAMELEN) {
                        fprintf(stderr, "%s: file names may not be larger than %i characters!\n", basename(argv[0]), MAXFILENAMELEN);
                        errFlg = true;
                        break;
                    }
                    strcpy(files[nFiles++], fileName);
                }
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
    unsigned int *workers;                                                                                      /* counting variable */
    int *pStatus;                                                                       /* pointer to execution status */

    /* initializing the application defined thread id arrays for the workers and the random number
        generator */

    if (((tIdWorkers = malloc(nThreads * sizeof (pthread_t))) == NULL) ||
        ((workers = malloc(nThreads * sizeof (unsigned int))) == NULL)) {
        fprintf(stderr, "error on allocating space to both internal / external worker id arrays\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nThreads; i++) workers[i] = i;

    srandom ((unsigned int) getpid());
    (void) get_delta_time();

    // store file names in shared memory
    storeFileNames(files);

    // create worker threads
    for (int i = 0; i < nThreads; i++)
    if(pthread_create (&tIdWorkers[i], NULL, worker, &workers[i]) != 0) { 
        perror("Error on creating thread worker.");
        exit(EXIT_FAILURE);
    }

    // wait for workers to finish
    for (int i = 0; i < nThreads; i++) { 
        if (pthread_join(tIdWorkers[i], (void *) &pStatus) != 0) {
            perror("error on waiting for thread producer");
            exit(EXIT_FAILURE);
        }
        printf("Thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *pStatus);
    }

    // print results
    printResults();

    printf ("\nElapsed time = %.6f s\n", get_delta_time ());

    return 0;
}

/**
 * @brief Worker funtion.
 * 
 * Its role is to simulate the life cycle of a worker.
 * 
 * @param args pointer to application defined worker identification
 * @return void* 
 */
static void *worker(void *args) {
    // Worker ID
    unsigned int id = *((unsigned int *) args);

    // Allocate memory for text chunk
    unsigned char * chunk;
    if((chunk = (unsigned char *)malloc(MAXTEXTSIZE * sizeof(unsigned char))) == NULL) {
        perror("Error on allocating memory.");
        statusWorker[id] = EXIT_FAILURE;
        pthread_exit(&statusWorker[id]);
    }

    bool quit = false;

    while(true) {
        // Worker fetches new text chunk
        int chunkSize;
        quit = readFromFile(id, chunk, &chunkSize);

        if(quit) break;

        // Process text chunk, count vowel occurence in words and word count
        unsigned int wordCountPartial = 0;
        unsigned int vowelCountPartial[VOWELNUM];
        bool firstOccur[VOWELNUM];
        for(int i = 0; i < VOWELNUM; i++) {
            vowelCountPartial[i] = 0;
            firstOccur[i] = false;
        }
        bool inWord = false;
        unsigned int letterCount = 0;
        for(int i = 0; i < chunkSize; i++) {
            letterCount++;
            unsigned char byte = chunk[i];
            unsigned char letter[4] = {byte, 0, 0, 0};
            int size = getLetterSize(byte);
            for(int j = 1; j < size; j++) letter[j] = chunk[++i];

            if(inWord) {
                if(isSeparator(letter, size)) inWord = false;
                else {
                    unsigned char character = isVowel(letter, size);
                    for(int j = 0; j < VOWELNUM; j++) {
                        if(character == vowels[j] && !firstOccur[j]) {
                            vowelCountPartial[j]++;
                            firstOccur[j] = true;
                        }
                    }
                }
            }
            else {
                if(isAlpha(letter, size) || ((letter[0] == 0x5F) && (size = 1))) {
                    wordCountPartial++;
                    inWord = true;
                    unsigned char character = isVowel(letter, size);
                    for(int j = 0; j < VOWELNUM; j++) {
                        firstOccur[j] = false;
                        if(character == vowels[j]) {
                            vowelCountPartial[j]++;
                            firstOccur[j] = true;
                        }
                    }
                }
            }
        }

        // Update counting varibales with partial results
        updateCounts(id, wordCountPartial, vowelCountPartial);
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
