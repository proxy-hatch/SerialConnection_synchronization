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
				  // Positive: buffer at file descriptor filled with char awaiting to be read
				  // Negative: Invalid. exception
				  // Zero: buffer empty (ready to accept new write requests from either direction)
	mutable std::mutex buffMut;	// used to update buffCnter variable upon read()/write(), as well as control wait()s and notify()s
	mutable std::condition_variable cv;	// used to wait and notify threads calling myTcdrain()

	// constructor
	sockInfo(const int fildes) :
			fd(fildes), buffCnter(0) {
	}
}/*socketPairInfo*/;
// typedef not needed anymore in C++: https://stackoverflow.com/a/252867/5340330

// instantiate vector container
std::vector<sockInfo*> sockList(DEFAULT_CONTAINER_SIZE);
std::mutex vectorMut;// used for opening, creating, or closing file descriptors.
// Shared amongst all the objects in the vector.

// |-------------------------------------------------------------------------|
// |                          Helper Functions                               |
// |-------------------------------------------------------------------------|

// Retrieve the socketPairInfo dynamic struct related to files
sockInfo* getSockInfo(const int fd) {
	if (fd < 0) {
#ifdef _DEBUG
		std::cerr << "getSockInfo(" << fd << ") invalid: Negative."
				<< std::endl;
#endif
		return nullptr;
	}
	sockInfo* sock;
	// search for socket
	try {
		sock = sockList.at(fd); // vector::at() throws an out-of-range
	} catch (const std::out_of_range& oor) {
#ifdef _DEBUG
		std::cerr << "Out of Range error: " << oor.what() << std::endl;
#endif
		return nullptr;
	}

	return sock;	// may return nullptr
}

// insert sockInfo entry at vector.at(fd)
// return 0 upon success.
// return 1 if there is an error
int addSockInfo(const sockInfo* newSock, const int fd) {
	sockInfo* sock;

	// error handle: fd out of range
	try {
		sock = sockList.at(fd); // vector::at() throws an out-of-range
	} catch (const std::out_of_range& oor) {
#ifdef _DEBUG
		std::cerr << "Out of Range error at addSockInfo(" << fd << "): "
				<< oor.what() << std::endl;
#endif
		return 1;
	}

	// error handle: sockInfo already exist
	if (sock) {
#ifdef _DEBUG
		std::cerr << "addSockInfo(" << fd
				<< ") failed: sockInfo already created for fd" << std::endl;
#endif
		return 1;
	}

	// Lock vector upon modifying
	std::lock_guard<std::mutex> lk(vectorMut);
	sockList.at(fd) = const_cast<sockInfo*>(newSock); // compiler error of violating const of newSock: Force write with const_cast
													  // https://stackoverflow.com/a/731130
	return 0;
}

// delete sockInfo entry at vector.at(fd)
// return 0 upon success
// return 1 if there is an error
int delSockInfo(const int fd) {
	sockInfo* sock;
	// error handle: fd out of range
	try {
		sock = sockList.at(fd); // vector::at() throws an out-of-range
	} catch (const std::out_of_range& oor) {
#ifdef _DEBUG
		std::cerr << "Out of Range error at addSockInfo(" << fd << "): "
				<< oor.what() << std::endl;
#endif
		return 1;
	}

	// Lock vector upon modifying
	if (sock) {
		std::lock_guard<std::mutex> lk(vectorMut);
		sockList.at(fd) = nullptr;
	}
	// 'delete' called outside lock_guard scope to avoid unnecessary overhead
	if (sock)
		delete sock;

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
		if (addSockInfo(newSock0, des[0])) {
			// error handle
			returnVal = -1;
		} else if (addSockInfo(newSock1, des[1])) {
			// delete first one if it did not give error
			if (returnVal != -1)
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
	// get socket info (return nullptr if not a socket)
	sockInfo* readSock = getSockInfo(fildes);

	// no need to check if fildes is an actual socket because myReadcond() supports non-socket reads
	// no need to check if fildes is in-use because myReadcond() also handle read fails
	// If myReadcond() is blocked waiting to read,and then the socket closes. It will return with -1 (no hang)
	int retVal = myReadcond(fildes, buf, nbyte, 1, 0, 0);

	if (retVal == -1 || !readSock) {	// error or non-socket
#ifdef _DEBUG
		if (retVal == -1)
			std::cerr << "myRead(" << fildes << ", buf, " << nbyte << ") failed"
					<< std::endl;
#endif _DEBUG
		return retVal;
	}

	// at least 1 byte was read. lock and update buffCnter
	// We are making the assumption here that in between myReadcon() returning and the following line, there has been no new write to fd.
	/* this is a valid assumption because:
	 * if myWrite(fd) happened before myReadcond(fd) was called, myReadcond(fd) would immediately return, becoming the next to hold mutex
	 * If myWrite(fd) was called after myReadcond(fd) was called, myReadcond(fd) would immediately return  after myWrite(fd) finishes, becoming the next to hold mutex
	 * having ANOTHER writer acquiring mutex in between - the myWrite(fd) mentioned above releasing mutex and myReadcond(fd) acquiring mutex - is highly unlikely.
	 * having ANOTHER tcDrain() thread acquiring mutex in between - does not pose a contradiction.
	 */
	{
		std::lock_guard<std::mutex> lk(readSock->buffMut);
		readSock->buffCnter = readSock->buffCnter - retVal;
	}

	// While holding the same mutex for notify()ing seems necessary for precise scheduling,
	// (i.e., tcDrain()::wait()s should happen before or after the notify(), not at the same time.)
	// it is inefficient to do so.	see for more: https://stackoverflow.com/a/17102100
	// Therefore we call notify_all only after immediate release of mutex
	/* "
	 * The notifying thread does not need to hold the lock on the same mutex as the one held by the waiting thread(s);
	 * in fact doing so is a pessimization, since the notified thread would immediately block again, waiting for the notifying thread to release the lock
	 * ...... Notifying while under the lock may nevertheless be necessary when precise scheduling of events is required
	 * " - http://en.cppreference.com/w/cpp/thread/condition_variable/notify_one
	 */
	// notify_all instead of notify_one is called here to handle the case of potentially multiple tcDrain waiting on fd.
	readSock->cv.notify_all();

	return retVal;
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
	sockInfo* sock = getSockInfo(des);

	// while reading buffCnter does not cause concurrency issues, it is necessary to hold the mutex before reading,
	// in case of buffCnter being updated after reading,  causing invalid judgement of whether to wait or not.
	// e.g., (buffCnter!=0) == true --> myRead() made buffCnter=0 and notify_all() --> myTcdrain() misses notify_all() and now waits despite buffCnter==0
	std::unique_lock<std::mutex> lk(sock->buffMut);
	if (sock->buffCnter)
		sock->cv.wait(lk, [] {return !(sock->buffCnter);});

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

