/* Additional QNX-like functions.  Simon Fraser University -- Copyright (C) 2008-2011, By
 *  - Craig Scratchley
 *  - Zhenwang Yao.
 */

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#endif

#include <sys/socket.h>
#include <sys/time.h>	// for timeval ???
#include <stdio.h>	    // fprintf()
#include <fcntl.h>
#include <errno.h>
#include "VNPE.h"

// a version of readcond() that works with both terminal devices and socket(pair)s
int wcsReadcond( int fd,
              void * buf,
              int n,
              int min,
              int time,
              int timeout )
{
    int bytesRead;
    int errnoHold;
    struct timeval tv, tvHold;
    int minHold;
    socklen_t minLenHold;
    socklen_t tvLenHold=sizeof(tvHold);

    if (-1 == getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tvHold, &tvLenHold)) {
#ifdef __QNXNTO__
    	if (errno == ENOTSOCK) /* Socket operation on non-socket - documented? */
    		// the fd is not a socket, try normal readcond
    		return readcond( fd, buf, n, min, time, timeout);
    	else
#endif
    	{
    		// some errors, like bad descriptor, should just be returned.
    		//VNS_errorReporter ("1st getsockopt() in wcsReadcond()", __FILE__, __func__, __LINE__, errno, NULL);
    		return -1;
    	};
    }
    if (time != timeout) {
    	fprintf(stderr, "wcsReadcond() requires for sockets that time == timeout\n");
    	errno = EINVAL;
    	return -1;
    };

    minLenHold=sizeof(minHold);
    PE(getsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &minHold, &minLenHold)); // should we return if -1?

    if (min != 0) {
        int bytesSoFar = 0;

    	if (time != 0) {
		    // add timeout to read() call
		   	tv.tv_sec = time/10;
		   	tv.tv_usec = time%10 *100000;
		   	PE(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));
    	};

	    // set socket to grab min bytes before unblocking
	    PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &min, sizeof(min)));

	    do {
			do {
				// need to update timeout
				bytesRead = read(fd, ((char*) buf) + bytesSoFar, n - bytesSoFar);
			}
			while ((bytesRead == -1 && errno == EINTR));
			if (bytesRead == -1) {
				errnoHold = errno;
				break;
			}
			else if (bytesRead)
				bytesSoFar += bytesRead;
			else
				break;
	    }
	    while (bytesSoFar < min);
	    if (bytesRead != -1)
	    	bytesRead = bytesSoFar;
    }
	if( min == 0 || (bytesRead == -1 && errnoHold == EWOULDBLOCK) ) {
		//timeout occurred, attempt to read everything off the pipe with
		//a non-blocking read()
		//if nothing is in the pipe, timeout will trigger and set bytesRead to -1

		int lowat=1;
		int flags;
		PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &lowat, sizeof(lowat))); // should we return if -1?

		//set non-blocking
		if( -1==(flags=PE(fcntl(fd, F_GETFL, 0))) ) {
    		return -1;  // if PE doesn't exit()
		}
		if( -1 == PE(fcntl(fd, F_SETFL, flags | O_NONBLOCK)) ) {
    		return -1;  // if PE doesn't exit()
		}

		bytesRead = read(fd, buf, n);
		if( bytesRead == -1) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				// check with readcond to verify this is the behaviour we want.
				bytesRead = 0;
			}
			else
				errnoHold = errno;
		}
		//set blocking
		if( -1 == PE(fcntl(fd, F_SETFL, flags)) ) {
    		return -1;  // if PE doesn't exit()
		}
	}

	//reset the socket
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tvHold, sizeof(tvHold))) {
		if (errno != EBADF) { // Bad file descriptor (9).  Socket was closed
			VNS_errorReporter ("setsockopt() resetting the socket", __FILE__, __func__, __LINE__, errno, NULL);
			// should we do the other setsockopt here?
			return -1; // if errorReporter doesn't exit()
		};
	}
	else
		PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &minHold, sizeof(minHold)));

	if (bytesRead == -1)
		errno = errnoHold;   // does this work?  I saw in debugger on QNX after this line executed errno = 0, errnoHold = 11
   	return bytesRead;
}

