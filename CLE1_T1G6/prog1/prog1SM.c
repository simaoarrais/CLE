/**
 * @file prog1SM.c (implementation file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Vowel Count.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the threads:
 *     \li (worker) readFromFile
 *     \li (worker) updateCounts
 *     \li (main) storeFileNames
 *     \li (main) printResults.
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

#include "prog1Utils.h"
#include "probConst.h"

/** \brief return status on monitor initialization */
extern int statusInitMon;

/** \brief worker threads return status array */
extern int *statusWorker;

/** \brief main/generator thread return status */
extern int statusMain;

/** \brief number of files input by the user */
extern int nFiles;

/** \brief number of threads input by the user */
extern int nThreads;

// Shared memory
/** \brief array of word counts for each file */
static unsigned int * wordCount;

/** \brief 2D array of vowel count for each file and vowel */
static unsigned int ** vowelCounts;

/** \brief array of file name for each file */
static char ** fileNames;

/** \brief array of pointers signaling the bytes already processed for each file */
static unsigned int * fileBuffer;

/** \brief boolean array signaling if a file is done being processed */
static bool * fileOver;

/** \brief file currently being processed */
static int currFile;

/** \brief array of pointers to the file each worker is processing */
static int * currFileWorker;

/** \brief flag signaling if the work is finished (all files processed) */
static bool workFinished;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief flag which warrants that the data transfer region is initialized exactly once */
static pthread_once_t init = PTHREAD_ONCE_INIT;

// TODO: do we need init fileNames (already allocated on main)
/**
 *  \brief Initialization of the data transfer region.
 *
 *  Internal monitor operation.
 */
static void initialization(void) {
    if(((wordCount = (unsigned int *)malloc(VOWELNUM * sizeof(unsigned int))) == NULL) ||
       ((vowelCounts = (unsigned int **)malloc(nFiles * sizeof(unsigned int *))) == NULL) ||
       ((fileBuffer = (unsigned int *)malloc(nFiles * sizeof(unsigned int))) == NULL) ||
       ((fileOver = (bool *)malloc(nFiles * sizeof(bool))) == NULL) ||
       ((fileNames = (char **)malloc(MAXFILECOUNT * sizeof(char *))) == NULL) ||
       ((currFileWorker = (int *)malloc(nThreads * sizeof(int))) == NULL)) {
        fprintf (stderr, "Error on allocating space to the data transfer region!\n");
        statusInitMon = EXIT_FAILURE;
        pthread_exit (&statusInitMon);
    }
    for(int i = 0; i < nFiles; i++) {
        if((vowelCounts[i] = (unsigned int *)malloc(VOWELNUM * sizeof(unsigned int))) == NULL) {
            fprintf (stderr, "Error on allocating space to the data transfer region!\n");
            statusInitMon = EXIT_FAILURE;
            pthread_exit(&statusInitMon);
        }
    }
    for(int i = 0; i < MAXFILECOUNT; i++) {
        if((fileNames[i] = (char *)malloc((MAXFILENAMELEN+1) * sizeof(char))) == NULL) {
            fprintf (stderr, "Error on allocating space to the data transfer region!\n");
            statusInitMon = EXIT_FAILURE;
            pthread_exit(&statusInitMon);
        }
    }

    for(int i = 0; i < nFiles; i++) {
        fileBuffer[i] = 0; // all file processing starts at the beginning of the file
        fileOver[i] = false; // initially, no files are processed
        for(int j = 0; j < VOWELNUM; j++) { // initialize word and vowel counts
            vowelCounts[i][j] = 0;
            wordCount[j] = 0;
        }
    }
    for(int i = 0; i < nThreads; i++) { // initialize pointers from workers to files
        currFileWorker[i] = 0;
    }
    currFile = 0; // processing starts with file with index 0
    workFinished = false;
}

/**
 * @brief Store file names in the data transfer region.
 * 
 * Operation carried out by the main thread after processing user input.
 * 
 * @param names array of file names to be stored
 */
void storeFileNames(char ** names) {
    statusMain = pthread_mutex_lock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread entering monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
    pthread_once(&init, initialization);

    fileNames = names;

    statusMain = pthread_mutex_unlock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread exiting monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
}

/**
 * @brief Retrieve a chunk of file text.
 * 
 * Operation carried out by the workers.
 * 
 * @param workerID worker identification
 * @param chunk output variable, stores the text chunk from the file
 * @param chunkSize output variable, stores the size, in bytes, of the text chunk
 * @return true : the worker's work is finished signaling it should quit 
 * @return false : the worker should continue it's life cycle
 */
bool readFromFile(unsigned int workerID, unsigned char * chunk, int * chunkSize) { // worker
    statusWorker[workerID] = pthread_mutex_lock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on entering monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    if(workFinished) { // files were all processed, move on and die
        statusWorker[workerID] = pthread_mutex_unlock(&accessCR); // leave monitor
        if(statusWorker[workerID]) {
            errno = statusWorker[workerID];
            perror("Error on leaving monitor (CF).");
            statusWorker[workerID] = EXIT_FAILURE;
            pthread_exit(&statusWorker[workerID]);
        }
        return true;
    } 

    // store worker's current file
    currFileWorker[workerID] = currFile;
    int currFileInit = currFile;

    // open file
    FILE * fp = fopen(fileNames[currFile], "r");

    // skip content already processed
    if((statusWorker[workerID] = fseek(fp, fileBuffer[currFile], SEEK_SET)) != 0) {
        fclose(fp);
        errno = statusWorker[workerID];
        perror("Error repositioning file buffer.");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    // read chunk of text
    size_t bytesRead = fread(chunk, 1, MAXTEXTSIZE, fp);

    // verify if text contains only full words
    if(bytesRead < MAXTEXTSIZE) { // read captured the file until its end, so nothing was cut
        fileOver[currFile] = true;
        fileBuffer[currFile] += bytesRead;
        *chunkSize = bytesRead;
    }
    else { // check if word/letter was cut in half
        // look forward, is it a separator character?
        unsigned char letter[4];
        fread(letter, 1, 4, fp);
        bool somethingCut = true;
        if((letter[0] & 0xC0) != 0x80) { // it's the start of an ASCII char, we didn't cut a character in half
            int size = getLetterSize(letter[0]);
            somethingCut = !isSeparator(letter, size); // but is it a separator? if not we might've cut a word in half
        }
        if(somethingCut) { // we cut something, time to uncut it (find the first separator inside the captured text chunk)
            int actualBytesRead;
            for(actualBytesRead = (int)bytesRead; actualBytesRead >= 0; actualBytesRead--) { // walking backwards
                if((chunk[actualBytesRead] & 0xC0) == 0x80) continue; // not the start of a letter, continue
                else {
                    int size = getLetterSize(chunk[actualBytesRead]);
                    for(int i = 0; i < size; i++) letter[i] = chunk[actualBytesRead + i]; 
                    int isSep = isSeparator(letter, size);
                    if(isSep) break; // found a separator, cut text chunk here
                }
            }
            // update variables for progress tracking
            fileBuffer[currFile] += actualBytesRead;
            *chunkSize = actualBytesRead;
        }
        else { // nothing was cut, update variables for progress tracking
            fileBuffer[currFile] += bytesRead;
            *chunkSize = bytesRead;
        }
    }

    if(fileOver[currFile]) { // if this file ended, move on to next file
        currFile++;
        if(currFile >= nFiles) { // no more files to process, work is done
            workFinished = true;    
        } 
    }

    fclose(fp);

    statusWorker[workerID] = pthread_mutex_unlock(&accessCR);
    if(statusWorker[workerID]) {
        // undo changes
        currFile = currFileInit;
        workFinished = false;
        fileBuffer[currFile] -= bytesRead;
        fileOver[currFile] = false;
        errno = statusWorker[workerID];
        perror("Error on leaving monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    return false;
}

/**
 * @brief Update word and vowel count for processed file.
 * 
 * Operation carried out by the consumers after processing a chunk.
 * 
 * @param workerID worker identification
 * @param wordCountPartial word count for processed text chunk
 * @param vowelCountsPartial vowel counts for processed text chunk
 */
void updateCounts(unsigned int workerID, unsigned int wordCountPartial, unsigned int * vowelCountsPartial) {
    statusWorker[workerID] = pthread_mutex_lock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on entering monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }

    // update counts for the file
    wordCount[currFileWorker[workerID]] += wordCountPartial;
    for(int i = 0; i < VOWELNUM; i++) {
        vowelCounts[currFileWorker[workerID]][i] += vowelCountsPartial[i]; 
    }

    statusWorker[workerID] = pthread_mutex_unlock(&accessCR);
    if(statusWorker[workerID]) {
        errno = statusWorker[workerID];
        perror("Error on entering monitor (CF).");
        statusWorker[workerID] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerID]);
    }
}

/**
 * @brief Print the current word and vowel counts.
 * 
 * Operation carried out by main thread after all workers have quit.
 * 
 */
void printResults() {
    statusMain = pthread_mutex_lock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread entering monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
    pthread_once(&init, initialization);

    for(int i = 0; i < nFiles; i++) {
        printf("File name: %s\n", fileNames[i]);
        printf("Total number of words = %i\n", wordCount[i]);
        printf("N. of words with an\n");
        printf("\tA\tE\tI\tO\tU\tY\n");
        printf("\t%i\t%i\t%i\t%i\t%i\t%i\n\n", vowelCounts[i][0], vowelCounts[i][1], vowelCounts[i][2], vowelCounts[i][3], vowelCounts[i][4], vowelCounts[i][5]);
    }

    statusMain = pthread_mutex_unlock(&accessCR);
    if(statusMain) {
        errno = statusMain;
        perror("Error on main thread exiting monitor (CF).");
        statusMain = EXIT_FAILURE;
    }
}
