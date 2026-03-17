/*

 * File: quinn_defs.h

 * Purpose: Header file for Quinn's parallel computing macros.

 * Required by Homework 3 instructions.

 */

#ifndef QUINN_DEFS_H

#define QUINN_DEFS_H

// Macros from Quinn's Textbook / Class PDF

#define MIN(a,b) ((a)<(b)?(a):(b))

// Given rank (id), number of processes (p), and size (n), returns the index of the first element

#define BLOCK_LOW(id, p, n)  ((id)*(n)/(p))

// Given rank (id), number of processes (p), and size (n), returns the index of the last element

#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1,p,n)-1)

// Returns the number of elements owned by the thread/process

#define BLOCK_SIZE(id, p, n) (BLOCK_HIGH(id,p,n)-BLOCK_LOW(id,p,n)+1)

// Returns the owner (rank) of index j

#define BLOCK_OWNER(j, p, n) (((p)*((j)+1)-1)/(n))

#endif

