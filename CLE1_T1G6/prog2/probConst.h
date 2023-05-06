/**
 * @file probConst.h
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Bitonic Integer Sorting.
 * 
 * Problem simulation parameters.
 * 
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef PROBCONST_H_
#define PROBCONST_H_

/** \brief maximum string length for a file name. */
#define MAXFILENAMELEN 30

/** \brief worker command enum: order non-bitonic sequence decreasing */
#define ORDER_NON_BITONIC_DCR -2
/** \brief worker command enum: order bitonic sequence decreasing */
#define ORDER_BITONIC_DCR -1
/** \brief worker command enum: worker is available (finished/waiting for work) */
#define AVAILABLE 0
/** \brief worker command enum: order bitonic sequence increasing */
#define ORDER_BITONIC_INCR 1
/** \brief worker command enum: order non-bitonic sequence increasing */
#define ORDER_NON_BITONIC_INCR 2
/** \brief worker command enum: worker's work is done */
#define DIE 3


#endif /* PROBCONST_H_ */
