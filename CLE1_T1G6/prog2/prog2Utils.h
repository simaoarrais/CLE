/**
 * @file prog2Utils.h (interface file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Bitonic Integer Sorting.
 * 
 * Utility functions that implement bitonic sort.
 * 
 * Functions: 
 *     \li bitonicMerge
 *     \li bitonicSort.
 *  
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef PROG2_UTILS_H
#define PROG2_UTILS_H

/**
 * @brief Merge the two halves of a bitonic sequence in a given order.
 * 
 * @param sequence pointer to the bitonic sequence
 * @param low index of the starting element
 * @param N number of elements to be sorted
 * @param dir sorting order, positive for increasing
 * @return int : exit status
 */
extern int bitonicMerge(int ** sequence, int low, int N, int dir);

/**
 * @brief Sort non bitonic sequence with bitonic sort.
 * 
 * @param sequence pointer to the sequence
 * @param low index of the starting element
 * @param N number of elements to be sorted
 * @param dir sorting order, positive for increasing
 * @return int : exit status
 */
extern int bitonicSort(int ** sequence, int low, int N, int dir);

#endif