#ifndef CONSTANTS_H
#define CONSTANTS_H


/** \brief maximum number of files allowed */
#define MAX_NUM_FILES 10

/** \brief maximum number of files allowed */
#define MAX_NUM_THREADS 8

/** \brief MPI tag to identify if the program is done (no more tasks to do) */
#define MPI_TAG_PROGRAM_STATE 0

/** \brief MPI tag to identify a chunk request */
#define MPI_TAG_CHUNK_REQUEST 1

/** \brief MPI tag to identify a message with partial results from a chunk */
#define MPI_TAG_SEND_RESULTS 2

/** \brief Identify an unsorted sequence */
#define SEQUENCE_UNSORTED 0

/** \brief Identify an unsorted sequence */
#define SEQUENCE_SORTED 1

/** \brief Identify a sequence that is currently being merged */
#define SEQUENCE_BEING_MERGED 2

/** \brief Identify a merged sequence (the sequence was merged and sorted into another sequence and is no longer needed) */
#define SEQUENCE_OBSOLETE 3

/** \brief Identify the sequence with all the file numbers sorted */
#define SEQUENCE_FINAL 4


#endif /* CONSTANTS_H */
