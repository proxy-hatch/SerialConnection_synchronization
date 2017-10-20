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
	const int fd;
	int pairedFd;	// set to -1 if the paired socket gets close()d.
	int buffCnter;// used to keep track of how many chars are inside the buffer at a given moment
				  // Positive: buffer at file descriptor filled with char awaiting to be read
				  // Negative: Invalid. exception
				  // Zero: buffer empty (ready to accept new write requests from either direction)
	mutable std::mutex buffMut;	// used to update buffCnter variable upon read()/write(), as well as control wait()s and notify()s
	mutable std::condition_variable cv;	// used to wait and notify threads calling myTcdrain()

	// constructor
	sockInfo(const int fildes, const int pairedFd) :
			fd(fildes), pairedFd(pairedFd), buffCnter(0) {
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
int addSockInfo(const sockInfo* newSock) {
	if(!newSock){
#ifdef _DEBUG
		std::cerr << "sockInfo obj ptr passed invalid" << std::endl;
#endif
		return 1;
	}

	sockInfo* sock;

	// error handle: fd out of range
	try {
		sock = sockList.at(newSock->fd); // vector::at() throws an out-of-range
	} catch (const std::out_of_range& oor) {
#ifdef _DEBUG
		std::cerr << "Out of Range error at addSockInfo(" << newSock->fd << "): "
				<< oor.what() << std::endl;
#endif
		return 1;
	}

	// error handle: sockInfo already exist
	if (sock) {
#ifdef _DEBUG
		std::cerr << "addSockInfo(" << newSock->fd
				<< ") failed: sockInfo already created for fd" << std::endl;
#endif
		return 1;
	}

	// Lock vector upon modifying
	std::lock_guard<std::mutex> lk(vectorMut);
	sockList.at(newSock->fd) = const_cast<sockInfo*>(newSock); // compiler error of violating const of newSock: Force write with const_cast
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
		sockInfo* newSock0 = new sockInfo(des[0],des[1]);
		sockInfo* newSock1 = new sockInfo(des[1],des[0]);

		// add vector entry for each socket
		// if the first one returned non-zero, do not proceed with the second one
		// if the second one returned with non-zero, delete the first one
		// Theoretically neither socket should be occupied already
		if (addSockInfo(newSock0)) {
			// error handle
			returnVal = -1;
		} else if (addSockInfo(newSock1)) {
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

	if (retVal == -1 || !readSock) {	// error or non-socket: do not continue
#ifdef _DEBUG
		if (retVal == -1)
			std::cerr << "myRead(" << fildes << ", buf, " << nbyte << ") failed"
					<< std::endl;
#endif
	} else {
		// at least 1 byte was read. lock and update buffCnter
		// We are making the assumption here that in between myReadcon() returning and the following line, there has been no new write to fd.
		/* this is a valid assumption because:
		 * if myWrite(fd) happened before myReadcond(fd) was called, myReadcond(fd) would immediately return, becoming the next to hold mutex
		 * If myWrite(fd) was called after myReadcond(fd) was called, myReadcond(fd) would immediately return after myWrite(fd) finishes, becoming the next to hold mutex
		 * having ANOTHER writer acquiring mutex in BETWEEN - the myWrite(fd) mentioned above releasing mutex and myReadcond(fd) acquiring mutex - is highly unlikely.
		 * having ANOTHER tcDrain() thread acquiring mutex in between - does not pose a contradiction.
		 * having ANOTHER tcDrain() thread acquiring mutex AFTER myReacond() returns - does not pose a contradiction either
		 *
		 */
		{
			std::lock_guard<std::mutex> lk(readSock->buffMut);
			readSock->buffCnter = readSock->buffCnter - retVal;


			// Holding buffMut for notify()ing is NOT inefficient because we are not holding pairedSock->buffMut
			// (see how it CAN be inefficient: http://en.cppreference.com/w/cpp/thread/condition_variable/notify_all
			// and https://stackoverflow.com/a/17102100).
			// It is essential for precise scheduling:
			/* Consider the following scenario (if notify_all() occurs after releasing buffMut):
			 * threadA::mySend(pairedFd) && set buffCnter to n bytes -> threadA::tcDrain()::wait(pairedFd) -> threadB::myRead(fd) resets buffCnter=0 and release buffMut
			 *  -> threadC::mySend(pairedFd) && set buffCnter to n bytes again -> threadC::tcDrain::wait(pairedFd) -> threadB::myRead()::notify_all().
			 * Now, both threadA AND threadC are woken up, even though buffCnter != 0 and threadC shouldn't be.
			*/
			if (!(readSock->buffCnter)) {
				sockInfo* pairedSock = getSockInfo(readSock->pairedFd);
				if(pairedSock){		// pairedSock==nullptr --> socket already closed. Nothing to do.
					// notify_all instead of notify_one is called here to handle the case of potentially multiple tcDrain waiting on fd.
					// note that we do not need to hold pairedSock->buffMut
					pairedSock->cv.notify_all();
				}
			}
		}
	}
	return retVal;
}

ssize_t myWrite(int fildes, const void* buf, size_t nbyte) {\
	ssize_t retVal;
	sockInfo* writeSock = getSockInfo(fildes);
	sockInfo* pairedSock;

	// fd is a socket: Hold bufMut before write(), and update pairedSock->buffCnter accordingly afterwards
	if (writeSock && (pairedSock = getSockInfo(writeSock->pairedFd))) {
		std::lock_guard<std::mutex> lk(pairedSock->buffMut);
		retVal = write(fildes, buf, nbyte);
		// update buffCnter as necessary
		if (retVal > 0) {
			pairedSock->buffCnter += retVal;
		}
	} else
		retVal = write(fildes, buf, nbyte);

	return retVal;
}

// closes the socket associated with fd but DO NOT close its paired socket.
// However, threads that are waiting on tcDrain(pairedFd)::pairedSock->cv.wait() should be waken up.
int myClose(int fd) {
	int retVal;
	sockInfo* closeSock = getSockInfo(fd);
	sockInfo* pairedSock;

	if (closeSock) {
		{
			// close() should not happen concurrently with read()/write()/wait(), therefore hold mutex
			std::lock_guard<std::mutex> lk(closeSock->buffMut);
			retVal = close(fd);
			closeSock->buffCnter = 0;	// reset buffCnter to prevent subsequent tcDrain(fd)::wait()
		}		// fd is now closed. Subsequent write()/read() should fail.
		closeSock->cv.notify_all();	// free the remaining tcDrain(fd)s

		// make paired socket aware that this socket is closed
		// It should NOT however, wake the threads blocked by tcDrain(pairedFd). That is the responsibility of myRead(pairedFd)
		if ((pairedSock = getSockInfo(closeSock->pairedFd))) {
			pairedSock->pairedFd = -1;
		}

		// delete sockInfo* from vector to make room for the potentially re-purposed fd
		std::unique_lock<std::mutex> lk(vectorMut);
		sockList.at(closeSock->fd) = nullptr;
		lk.unlock();

		// Socket should now be safe to delete
		delete closeSock;
	} else
		retVal = close(fd);

	return retVal;
}

// myTcdrain() blocks the calling thread until all previously written characters have actually been sent
int myTcdrain(int des) { //is also included for purposes of the course.
	// check if des is indeed a socket
	if (sockInfo* sock = getSockInfo(des)) {
		// while reading buffCnter does not cause concurrency issues, it is necessary to hold the mutex before reading,
		// in case of buffCnter being updated after reading,  causing invalid judgement of whether to wait or not.
		// e.g., (buffCnter!=0) == true --> myRead() made buffCnter=0 and notify_all() --> myTcdrain() misses notify_all() and now waits despite buffCnter==0
		std::unique_lock<std::mutex> lk(sock->buffMut);
		if (sock->buffCnter) {
//			sock->cv.wait(lk, [] {return !(sock->buffCnter);});
			// while having the buffCnter check here thru the lambda seems like a good idea, we need myClose::notify_all() to be able to wake up tcDrain
			// Thus the responsibility of buffCnter checking relies on myRead::notify_all()
			sock->cv.wait(lk);
		}
	}
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

