/**
 * @file prog1SM.h (interface file)
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
#ifndef PROG2_SM_H
#define PROG2_SM_H

#include <stdbool.h>

/**
 * @brief Store file name in the data transfer region.
 * 
 * Operation carried out by the main thread after processing user input.
 * 
 * @param fileName name of the file to be stored
 */
extern void storeFileName(char * fileName);

/**
 * @brief Open the file and store the sequence and its size in shared memory.
 * 
 * Operation carried out by the distributor thread on initialization.
 * 
 */
extern void readFromFileAndStore(); 

/**
 * @brief Distribute sequence ranges and commands to the various workers.
 * 
 * Operation carried out by the distributor thread.
 * 
 * Command and ranges are assigned to each worker, then the thread awaits the work to be finished and reduces de number of active workers in half.
 * 
 * @param activeWorkers number of workers currently still alive
 */
extern void distributeRanges(int * activeWorkers);

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
extern bool fetchSubSequence(unsigned int workerID, int * command, int * chunkSize, int ** chunk);

/**
 * @brief Signal sorting is finished and was successful.
 * 
 * Operation carried out by the workers after successful sorting of the assigned sequence.
 * 
 * @param workerID worker identification
 */
extern void signalFinished(unsigned int workerID);

/**
 * @brief Validate if the sequence was properly sorted.
 * 
 * Operation carried out by main thread after all workers have quit.
 * 
 */
extern void validateSequence();

#endif