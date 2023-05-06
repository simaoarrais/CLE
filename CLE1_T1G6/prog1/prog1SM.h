/**
 * @file prog1SM.h (interface file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Vowel Count.
 * 
 * Synchronization based on monitors.
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
#ifndef PROG1_SM_H
#define PROG1_SM_H

#include <stdbool.h>

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
extern bool readFromFile(unsigned int workerID, unsigned char * chunk, int * chunkSize);

/**
 * @brief Update word and vowel count for processed file.
 * 
 * Operation carried out by the consumers after processing a chunk.
 * 
 * @param workerID worker identification
 * @param wordCountPartial word count for processed text chunk
 * @param vowelCountsPartial vowel counts for processed text chunk
 */
extern void updateCounts(unsigned int workerID, unsigned int wordCountPartial, unsigned int * vowelCountPartial);

/**
 * @brief Store file names in the data transfer region.
 * 
 * Operation carried out by the main thread after processing user input.
 * 
 * @param names array of file names to be stored
 */
extern void storeFileNames(char ** names);

/**
 * @brief Print the current word and vowel counts.
 * 
 * Operation carried out by main thread after all workers have quit.
 * 
 */
extern void printResults();

#endif
