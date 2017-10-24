/*
 * AtomicCOUT.h
 */
// Copyright © 2010 W. Craig Scratchley, craig_scratchley (at) alumni.sfu.ca
// School of Engineering Science, SFU, BC, Canada  V5A 1S6
// Copying and distribution of this file, with or without modification,
//     are permitted in any medium without royalty provided the copyright
//     notice and this notice are preserved.

#include "ScopedMutex.h"
#include "AtomicConsole.h"
#include <iostream>

// this macro can be used for C++ console output where you don't want
// 	multiple threads interleaving their chained insertion operations.
//	It is used like:
//		COUT << "Hello " << "World." << endl;
//	Thanks to Javier *** for the idea.
//  This may not be the optimal way to acheive the desired effect.  It
//	might be better for each thread to build a stringStream referenced from TLS and
//  flush the stringStream when endl or flush is encountered.
#define COUT (ScopedMutex(&consoleMutex), std::cout)

#define CERR (ScopedMutex(&consoleMutex), std::cerr)
