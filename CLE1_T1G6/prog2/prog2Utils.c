/**
 * @file prog2Utils.c (implementation file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Bitonic Integer Sorting.
 * 
 * Utility functions that implement bitonic sort.
 * 
 * Functions: 
 *     \li CAPS
 *     \li isSquare
 *     \li bitonicMerge
 *     \li bitonicSort.
 *  
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/**
 * @brief Compare and possibly switch two elements in an array, based on a given direaction.
 * 
 * @param sequence pointer to the sequence
 * @param pos1 index of an element to be compared 
 * @param pos2 index of an element to be compared 
 * @param dir sorting order, positive for increasing
 * @return int 
 */
int CAPS(int ** sequence, int pos1, int pos2, int dir) {
    if(dir >= 0) {
        if(*(*sequence + pos1) > *(*sequence + pos2)) {
            int tmp = *(*sequence + pos1);
            *(*sequence + pos1) = *(*sequence + pos2);
            *(*sequence + pos2) = tmp;
        }
    } 
    else if(*(*sequence + pos1) < *(*sequence + pos2)) {
        int tmp = *(*sequence + pos1);
        *(*sequence + pos1) = *(*sequence + pos2);
        *(*sequence + pos2) = tmp;
    }
    return 0;
}

/**
 * @brief Check is a certain positive integer is square
 * 
 * @param n the integer to be evaluated
 * @return true : the integer is square
 * @return false : the integer is not square
 */
bool isSquare(int n) {
    int iVar;
    float fVar;
    fVar = sqrt((double)n);
    iVar = fVar;
    if(iVar==(int)fVar) return true;
    return false;
}

/**
 * @brief Merge the two halves of a bitonic sequence in a given order.
 * 
 * @param sequence pointer to the bitonic sequence
 * @param low index of the starting element
 * @param N number of elements to be sorted
 * @param dir sorting order, positive for increasing
 * @return int : exit status
 */
int bitonicMerge(int ** sequence, int low, int N, int dir) { // sequence is bitonic
    if((N >= 2) && !isSquare(N)) {
        printf("Bitonic merge is not possible for this array size (%i).\n", N);
        return 1;
    }

    int v  = N >> 1;
    int nL = 1;
    int k = (int)log2((double)N);
    for(int m = 0; m < k; m++) {
        int n = 0;
        int u = 0;
        while(n < nL) {
            for(int t = 0; t < v; t++) {
                // Compare and possible swap idx t+u and t+u+v
                CAPS(sequence, low+t+u, low+t+u+v, dir);
            }
            u += (v << 1);
            n += 1; 
        }
        v >>= 1;
        nL <<= 1;
    }
    return 0;
}

/**
 * @brief Sort non bitonic sequence with bitonic sort.
 * 
 * @param sequence pointer to the sequence
 * @param low index of the starting element
 * @param N number of elements to be sorted
 * @param dir sorting order, positive for increasing
 * @return int : exit status
 */
int bitonicSort(int ** sequence, int low, int N, int dir) {
    if (N <= 1) return 0;
    else if (!isSquare(N)) {
        printf("Bitonic sort is not possible for this array size (%i).\n", N);
        return 1;
    }

    int k = N/2;
    int err = 0;

    // create bitonic seq
    err += bitonicSort(sequence, low, k, 1);
    err += bitonicSort(sequence, low+k, k, -1);
    err += bitonicMerge(sequence, low, N, dir);
    if(err) return 1;
    return 0;
}