//============================================================================
//
//% Student Name 1: Yu Xuan (Shawn) Wang
//% Student 1 #: 301227972
//% Student 1 userid (email): yxwang (stu1@sfu.ca)
//
//% Student Name 2: Sheung Yau Chung
//% Student 2 #: 301236546
//% Student 2 userid (email): sychung (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
// All TAs and the collective effort of the entire class through course piazza forum (including Dr Scratchley)
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ENSC 351 forum on piazza.com
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : October 7th, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================


/* to provide the best searching efficiency and scalablility for multiple socketpairs, a class of socket (file descriptor) pair, their shared mutex, bufferCnter, and conditional variable will be introduced.
 * An STL::unordered_map is then used to keep track of the classes (efficiency analysis: https://stackoverflow.com/a/25458271/5340330). Specifically, each file descriptor will be used as a key, and their corresponding class addr ptr will be stored as value.
 * As file descriptors are unique, there will be no collision and thus not even a need for hashing!
 * This will guarantee O(1) search AND inserts, thus optimal for this assignment.
 */

#ifndef MYSOCKET_H_
#define MYSOCKET_H_

#include <unistd.h>
#include <sys/stat.h>


/*
int myCreat(const char *pathname, mode_t mode);
ssize_t myRead( int fildes, void* buf, size_t nbyte );
ssize_t myWrite( int fildes, const void* buf, size_t nbyte );
int myClose(int fd);
*/

/* int myOpen( const char * path,
          int oflag,
          ... )
; */


int myOpen(const char *pathname, int flags, mode_t mode);

int myCreat(const char *pathname, mode_t mode);
int mySocketpair( int domain, int type, int protocol, int des[2] );
ssize_t myRead( int des, void* buf, size_t nbyte );
ssize_t myWrite( int des, const void* buf, size_t nbyte );
int myClose(int des);
// The last two are not ordinarily used with sockets
int myTcdrain(int des); //is also included for purposes of the course.
int myReadcond(int des, void * buf, int n, int min, int time, int timeout);

#endif /*MYSOCKET_H_*/
