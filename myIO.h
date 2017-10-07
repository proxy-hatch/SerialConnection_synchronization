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
