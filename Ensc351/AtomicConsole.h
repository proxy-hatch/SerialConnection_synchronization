/*
 * AtomicConsole.h
 *
 *  Created on: Oct 7, 2010
 *      Author: wcs
 */

#ifndef ATOMICCONSOLE_H_
#define ATOMICCONSOLE_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

extern pthread_mutex_t consoleMutex;

#ifdef __cplusplus
}
#endif

#endif /* ATOMICCONSOLE_H_ */
