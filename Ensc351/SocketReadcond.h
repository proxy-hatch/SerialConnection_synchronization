/* Additional QNX-like functions.  Simon Fraser University -- Feb. 6, 2008, By
 *  - Craig Scratchley
 */

#ifndef SFUQNX_H_
#define SFUQNX_H_

#ifdef __cplusplus
extern "C"
{
#endif

	// a version of readcond() that works with both terminal devices and socket(pair)s
	int wcsReadcond( int fd, void * buf, int n, int min, int time, int timeout);

#ifdef __cplusplus
}
#endif

#endif /*SFUQNX_H_*/
