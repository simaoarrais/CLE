#ifndef UTILS_H
# define UTILS_H

/**
 * @brief Information about the file (name and pointer) and reference to the file sorted sequence (final state)
 * 
 */
struct fileInfo {
    char *filename;
    FILE *fp;
    unsigned int fileIndex;
    int numNumbers;
    unsigned int chunkSize;
    unsigned int *fullSequence;
    /*Array of integers to know if the sequence of that index is sorted (1) or not (0) */
    int sortedSequences[size-1]; 
    /**
        * Array of integers to know if the sequence of that index is merged with another sequence 
        * (>= 0 and the value corresponds to the index of the other sequence) or not (-2).
        * If the sequence was already merged into another sequence and is no longer needed, the
        * value is -1.
    */
    int mergedSequences[size-1] = {-2};   
    int isFinished;
};

/**
 * @brief Structure that stores the integer sequence, the size (number of integers of the sequence) and a boolean
 * variable *isSorted* to know if sequence is sorted (or not)
 * 
 */
struct Sequence {
    unsigned int sequence;
    unsigned int size;
    int status;
};


/** \brief get the determinant of given matrix */
extern double getDeterminant(int order, double *matrix); 

extern void bitonic_merge(int arr[], int low, int cnt, int dir);

extern void bitonic_sort_recursive(int arr[], int low, int cnt, int dir);

extern void bitonic_sort(int arr[], int n);

extern void merge_sorted_arrays(int *arr1, int n1, int *arr2, int n2, int *result);


#endif /* UTILS_H */
