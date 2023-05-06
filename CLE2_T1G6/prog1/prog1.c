/**
 * @file probConst.h
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Vowel Count.
 * 
 * TODO
 * 
 * @version 0.1
 * @date 2023-04-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <mpi.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "prog1Utils.h"

/** \brief maximum string length for a file name. */
#define MAXFILENAMELEN 30

/** \brief maximum number of files as input. */
#define MAXFILECOUNT 10

/** \brief maximum chunk size for each worker to read. */
#define MAXTEXTSIZE 4000

/** \brief number of defined vowels. */
#define VOWELNUM 6

/** \brief list of characters considered vowels */
static unsigned char vowels[6] = {'a', 'e', 'i', 'o', 'u', 'y'};

int main(int argc, char *argv[]) {
    int rank, totProc;

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &totProc);

    // Memory for dispatcher
    /** \brief array of file name for each file */
    char ** files;

    /** \brief number of files input by the user */
    int nFiles = 0;

    /** \brief array of word counts for each file */
    unsigned int * wordCount;

    /** \brief 2D array of vowel count for each file and vowel */
    unsigned int ** vowelCounts;

    /** \brief array of pointers signaling the bytes already processed for each file */
    unsigned int * fileBuffer;

    /** \brief boolean array signaling if a file is done being processed */
    bool * fileOver;

    /** \brief file currently being processed */
    int currFile;

    /** \brief array of pointers to the file each worker is processing */
    int * currFileWorker;

    // Memory for worker
    int chunkSize;

    // Memory for both
    unsigned char chunk[MAXTEXTSIZE];
    // if((chunk = (unsigned char *)malloc(MAXTEXTSIZE * sizeof(unsigned char))) == NULL) {
    //     printf(stderr, "Error on allocating memory for text chunk.");
    //     MPI_Finalize();
    //     return EXIT_FAILURE;
    // }

    /** \brief flag signaling if the work is finished (all files processed) */
    bool workFinished = false;

    if(rank == 0) { // init dispatcher structures
        int opt;

        if((files = (char **)malloc(MAXFILECOUNT * sizeof(char *))) == NULL) {
            fprintf(stderr, "Error allocating memory for file names.\n");
            return EXIT_FAILURE;
        }
        for(int i = 0; i < MAXFILECOUNT; i++) {
            if((files[i] = (char *)malloc((MAXFILENAMELEN+1) * sizeof(char))) == NULL) {
                fprintf(stderr, "Error allocating memory for file names.\n");
                return EXIT_FAILURE;
            }
        }

        // Process Command Line
        opterr = 0;
        do {
            bool errFlg = false;
            switch (opt = getopt(argc, argv, "f:")) {
                // case 't':
                //     if(atoi(optarg) <= 0) {
                //         fprintf(stderr, "%s: number of threads must be a positive integer!\n", basename(argv[0]));
                //         errFlg = true;
                //     }
                //     nThreads = (int) atoi(optarg);
                //     break;
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
            if(errFlg) {
                MPI_Finalize();
                return EXIT_FAILURE;
            }
        } while(opt != -1);
        if(argc == 1) {
            fprintf (stderr, "%s: invalid format\n", basename (argv[0]));
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        if(((wordCount = (unsigned int *)malloc(VOWELNUM * sizeof(unsigned int))) == NULL) ||
           ((vowelCounts = (unsigned int **)malloc(nFiles * sizeof(unsigned int *))) == NULL) ||
           ((fileBuffer = (unsigned int *)malloc(nFiles * sizeof(unsigned int))) == NULL) ||
           ((fileOver = (bool *)malloc(nFiles * sizeof(bool))) == NULL) ||
           ((currFileWorker = (int *)malloc((totProc-1) * sizeof(int))) == NULL)) {
            printf("Error on allocating space!\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }
        for(int i = 0; i < nFiles; i++) {
            if((vowelCounts[i] = (unsigned int *)malloc(VOWELNUM * sizeof(unsigned int))) == NULL) {
                printf("Error on allocating space to the vowel counts!\n");
                MPI_Finalize();
                return EXIT_FAILURE;
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
        for(int i = 0; i < (totProc-1); i++) { // initialize pointers from workers to files
            currFileWorker[i] = 0;
        }
        currFile = 0; // processing starts with file with index 0
        workFinished = false;
    }

    // Processing
    while(!workFinished) {
        if(rank == 0) {
            for(int currWorker = 1; currWorker < totProc; currWorker++) {
                MPI_Send(&workFinished, sizeof(bool), MPI_C_BOOL, currWorker, 0, MPI_COMM_WORLD);
                if(!workFinished) {
                    currFileWorker[currWorker-1] = currFile;
                    // int currFileInit = currFile;

                    FILE * fp = fopen(files[currFile], "r");

                    if(fseek(fp, fileBuffer[currFile], SEEK_SET) != 0) {
                        fclose(fp);
                        perror("Error repositioning file buffer.");
                        MPI_Finalize();
                        return EXIT_FAILURE;
                    }

                    // read chunk of text
                    size_t bytesRead = fread(chunk, 1, MAXTEXTSIZE, fp);

                    // verify if text contains only full words
                    if(bytesRead < MAXTEXTSIZE) { // read captured the file until its end, so nothing was cut
                        fileOver[currFile] = true;
                        fileBuffer[currFile] += bytesRead;
                        chunkSize = bytesRead;
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
                            chunkSize = actualBytesRead;
                        }
                        else { // nothing was cut, update variables for progress tracking
                            fileBuffer[currFile] += bytesRead;
                            chunkSize = bytesRead;
                        }
                    }

                    if(fileOver[currFile]) { // if this file ended, move on to next file
                        currFile++;
                        if(currFile >= nFiles) { // no more files to process, work is done
                            workFinished = true;    
                        } 
                    }

                    fclose(fp);

                    // TODO send chunk and size to worker
                    MPI_Send(&chunk, sizeof(unsigned char) * MAXTEXTSIZE, MPI_CHAR, currWorker, 0, MPI_COMM_WORLD);
                    MPI_Send(&chunkSize, sizeof(int), MPI_INT, currWorker, 0, MPI_COMM_WORLD);

                }
                else {
                    currFileWorker[currWorker-1] = -1;
                }     
            }
        }
        else {
            // recieve work finished
            MPI_Recv(&workFinished, sizeof(bool), MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("%d: Recieved workFinished %d\n", rank, workFinished);
            if(!workFinished) {
                // recieve chunk and size
                MPI_Recv(&chunk, sizeof(unsigned char) * MAXTEXTSIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&chunkSize, sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // printf("%d: Recieved chunk and chunk size (%d)\n", rank, chunkSize);

                // processing
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

                // send counts to dispatcher
                MPI_Send(&wordCountPartial, sizeof(unsigned int), MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&vowelCountPartial, sizeof(unsigned int) * VOWELNUM, MPI_INT, 0, 0, MPI_COMM_WORLD);

                // check if work finished
                MPI_Recv(&workFinished, sizeof(bool), MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // printf("%d: Recieved workFinished %d\n", rank, workFinished);
            }
            else {
                break;
            }
        }

        if(rank == 0) {
            for(int currWorker = 1; currWorker < totProc; currWorker++) {
                if(currFileWorker[currWorker-1] != -1) {
                    int workerFile = currFileWorker[currWorker-1];
                    unsigned int wordCountPartial;
                    unsigned int vowelCountPartial[VOWELNUM]; 
                    MPI_Recv(&wordCountPartial, sizeof(unsigned int), MPI_INT, currWorker, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&vowelCountPartial, sizeof(unsigned int) * VOWELNUM, MPI_INT, currWorker, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    // printf("Disp: Recieved counts\n");

                    wordCount[workerFile] += wordCountPartial;
                    for(int i = 0; i < VOWELNUM; i++) {
                        vowelCounts[workerFile][i] += vowelCountPartial[i];
                    }
                    MPI_Send(&workFinished, sizeof(bool), MPI_C_BOOL, currWorker, 0, MPI_COMM_WORLD);
                }
            }
        }
    }

    if(rank == 0) {
        for(int i = 0; i < nFiles; i++) {
            printf("File name: %s\n", files[i]);
            printf("Total number of words = %i\n", wordCount[i]);
            printf("N. of words with an\n");
            printf("\tA\tE\tI\tO\tU\tY\n");
            printf("\t%i\t%i\t%i\t%i\t%i\t%i\n\n", vowelCounts[i][0], vowelCounts[i][1], vowelCounts[i][2], vowelCounts[i][3], vowelCounts[i][4], vowelCounts[i][5]);
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}