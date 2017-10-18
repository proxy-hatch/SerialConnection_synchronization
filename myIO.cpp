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
// Helpers: All TAs and the collective effort of the entire class through course piazza forum (including Dr Scratchley)
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
// Version     : October th, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

/* Wrapper functions for ENSC-351, Simon Fraser University, By
 *  - Craig Scratchley
 * 
 * These functions may be re-implemented later in the course.
 */

/* Note by Shawn:
 * The implementation currently design does NOT support any sort of (intentional) deadlock handling
 */
#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"
#include "myIO.h"
#include <mutex>
#include <condition_variable>
#include <vector>

#define DEFAULT_CONTAINER_SIZE 50
#define _DEBUG

#ifdef _DEBUG
#include <iostream>		// cerr
#endif



struct sockInfo {
	const int fd;	// negative file descriptor is invalid
	int buffCnter;// used to keep track of how many chars are inside the buffer at a given moment
				  // Positive: buffer filled with char awaiting to be read
				  // Negative: Invalid. exception
				  // Zero: buffer empty (ready to accept new write requests from either direction)
	mutable std::mutex mut;		// used to update buffCnter variable
	mutable std::condition_variable cond;// used to wait and sleep threads calling myTcdrain()
	mutable std::condition_variable cond;// used to wait and sleep threads calling myTcdrain()


	// constructor
	sockInfo(const int fildes) : fd(fildes), buffCnter(0){}
}/*socketPairInfo*/;
// typedef not needed anymore in C++: https://stackoverflow.com/a/252867/5340330

// instantiate vector container
std::vector<sockInfo*> sockList (DEFAULT_CONTAINER_SIZE);

// |-------------------------------------------------------------------------|
// |                          Helper Functions                               |
// |-------------------------------------------------------------------------|

// Retrieve the socketPairInfo dynamic struct related to files
sockInfo* getSockInfo(const int fd) {
	if (fd < 0) {
#ifdef _DEBUG
		std::cerr << "getSockInfo(" << fd << ") invalid: Negative." << std::endl;
#endif
		return nullptr;
	}
	sockInfo* sock;
	// search for socketPair
	try {
		sock = sockList.at(fd); // unordered_map::at throws an out-of-range
	} catch (const std::out_of_range& oor) {
#ifdef _DEBUG
		std::cerr << "Out of Range error: " << oor.what() << '\n';
#endif
		return nullptr;
	}

	return sock;
}

// insert sockInfo entry at vector.at(fd)
// return 0 upon success.
// return 1 if there is an error (index occupied for example)
int addSockInfo(sockInfo* newSock, int fd){
	// error handle
	if(sockList.at(fd)){
#ifdef _DEBUG
		std::cerr << "addSockInfo(" << fd << ") failed: sockInfo already created for fd." << std::endl;
#endif
		return 1;
	}
	sockList.at(fd) =newSock;
	return 0;
}

// |-------------------------------------------------------------------------|
// |                           User Functions                                |
// |-------------------------------------------------------------------------|
int mySocketpair(int domain, int type, int protocol, int des[2]) {
	// instantiate socketPair
	int returnVal = socketpair(domain, type, protocol, des);

	// success
	if (returnVal != -1) {
		// des should now be filled with correct file descriptor, so we extract them for our struct
		sockInfo* newSock0 = new sockInfo(des[0]);
		sockInfo* newSock1 = new sockInfo(des[1]);

		// add vector entry for each socket
		// if the first one returned non-zero, do not proceed with the second one
		// if the second one returned with non-zero, delete the first one
		// Theoretically neither socket should be occupied already
		if(addSockInfo(newSock0, des[0])){
			// error handle
			returnVal = -1;
		}
		else if(addSockInfo(newSock1, des[1])){
			// delete first one if it did not give error
			if(returnVal != -1)
				sockList.at(des[0]) = nullptr;
			// error handle
			returnVal = -1;
		}
	}
	return returnVal;
}

int myOpen(const char *pathname, int flags, mode_t mode) {
	return open(pathname, flags, mode);
}

int myCreat(const char *pathname, mode_t mode) {
	return creat(pathname, mode);
}

ssize_t myRead(int fildes, void* buf, size_t nbyte) {
	if(buffCnter)
	lock_guard
	sockInfo* sock = getSockInfo(fildes);

	return myReadcond(fildes, buf, nbyte, 1, 0, 0);
}

ssize_t myWrite(int fildes, const void* buf, size_t nbyte) {
	// check if there is someone holding mutex

	return write(fildes, buf, nbyte);
}

// After talking with different TAs, it seems that one myClose(3) should close down its paired socket as well
// i.e., calling myClose(4) as well
int myClose(int fd) {

	return close(fd);
}

// myTcdrain() blocks the calling thread until all previously written characters have actually been sent
int myTcdrain(int des) { //is also included for purposes of the course.
	return 0;
}

// as suggested in instruction document and by the TAs, myReadCond() is called inside of myRead()
// It is capable to set a min: 'min' and max: 'n'. min < n of course.
// It is also capable of setting timeout, but that is beyond the scope of this assignment
// Scenarios: buffer has x chars
// x > n: read n and return
// x < n && x > min: return x with x bytes read
// x < min: block until x = min. Return x with x bytes read
int myReadcond(int des, void * buf, int n, int min, int time, int timeout) {
	return wcsReadcond(des, buf, n, min, time, timeout);
}

