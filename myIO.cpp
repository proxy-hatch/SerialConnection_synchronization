/* Wrapper functions for ENSC-351, Simon Fraser University, By
 *  - Craig Scratchley
 * 
 * These functions may be re-implemented later in the course.
 */

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"

int mySocketpair( int domain, int type, int protocol, int des[2] )
{
	int returnVal = socketpair(domain, type, protocol, des);
	return returnVal;
}

int myOpen(const char *pathname, int flags, mode_t mode)
{
	return open(pathname, flags, mode);
}

int myCreat(const char *pathname, mode_t mode)
{
	return creat(pathname, mode);
}

ssize_t myRead( int fildes, void* buf, size_t nbyte )
{
	return read(fildes, buf, nbyte );
}

ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
	// Acquires a lock 
	
	// Call write()
	
	// Call myTcdrain() and wait until reader finish draining everything in the buffered 
	
	// Unlocks automatically when function goes out of scope  
	return write(fildes, buf, nbyte );
}

int myClose( int fd )
{
	return close(fd);
}

int myTcdrain(int des)
{ //is also included for purposes of the course.
	
	// Wait on conditional variable. Put writer thread to sleep, 
	// gives the lock to a reader thread, which will drain everything in the int buffered 
	
	// Notify a writer thread (use notifyall in the future) when buffered == 0
	
	return 0;
}

int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
	return wcsReadcond(des, buf, n, min, time, timeout );
}

