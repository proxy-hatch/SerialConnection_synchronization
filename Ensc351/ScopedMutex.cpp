/*
 * ScopedMutex.cpp
 */
// Copyright � 2010 W. Craig Scratchley, craig_scratchley (at) alumni.sfu.ca
// School of Engineering Science, SFU, BC, Canada  V5A 1S6
// Copying and distribution of this file, with or without modification,
//     are permitted in any medium without royalty provided the copyright
//     notice and this notice are preserved.

#include "ScopedMutex.h"
#include "VNPE.h"

//static pthread_mutex_t SMutex;

ScopedMutex::ScopedMutex(pthread_mutex_t* SMutexP)
:mutexP(SMutexP)
{
	PE_0(pthread_mutex_lock(mutexP));
}

ScopedMutex::~ScopedMutex() {
	PE_0(pthread_mutex_unlock(mutexP));
}
