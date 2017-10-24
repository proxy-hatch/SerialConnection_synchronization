// for use when editing parts of the code...
//#define ALLOW_PE_AT_FILE_LEVEL
#define REINITIALIZE_ERRNO

// definitions used for ENSC 351
#define HAVE_TLS	// QNX has TLS

/*
VNS_PE.h is part of the VectorNav Support Library http://vnsupport.sourceforge.net/

		Copyright (c) 2009 - 2015 Craig Scratchley (craig_scratchley@alumni.sfu.ca),
 	 	 	 	 	 	 	 Mark Marszal (mark@cocoanut.org)

 The VectorNav Support Library is free software: you can redistribute it and/or modify
 it under the terms of the Lesser GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 The VectorNav Support Library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 Lesser GNU General Public License for more details.

 You should have received a copy of the Lesser GNU General Public License
 along with the VectorNav Support Library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*! \file VNPE.h
 \brief Useful macros for detecting and reporting errors reported by function calls.

 The functions here allow you to control how the error reporting is conducted.
 The PE(), etc., macros call VNS_errorReporter() if an error is detected from a function call.

 */

#ifndef VNPE_H_
#define VNPE_H_

#include <errno.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*VNS_cleanup_t) (void);

typedef void VNS_eReporter_t
		(const char* functionCall, const char* file, const char *func, int line, int error, const char* info)
;

typedef VNS_eReporter_t *VNS_eReporterP_t;

void halt(void)
;

void haltExit(int returnCode)
;

/*! \fn int VNS_setErrorReporter(VNS_eReporter_t)

 \brief Sets the error Reporter to the function of your choice.

 This function is mainly to be used in conjunction with the PE() macro. If PE() detects a value of -1,
 Once all data within PE() has been written/displayed, PE() will call VNS_errorReporter() to report the error.
 You can use VNS_setErrorReporter() to report the errors the way you want. The default Reporter exits the process.
 Reporter must return void and accept a struct of VNSErrorData.

 \param VNS_eReporter_t Pointer to a function that you wish to be called when an error is detected or NULL to use the default Reporter
 \return 0
 */
int VNS_setErrorReporter(VNS_eReporterP_t eReporterFn)
;

VNS_eReporter_t VNS_ErrorPrinter;

/*! \fn void VNS_defaultErrorReporter (const char* functionCall, const char* file, const char *func, int line, int error, const char* info)

 \brief Default Error Reporter

 The default error Reporter exits the parent program with a status of -1. If this is not desired,
 create your own function and use VNS_setErrorReporter() to change the default Reporter.

 \ **param **Various Parameters** Details about why the function failed. This data is passed from the PE*() Macros
 */
VNS_eReporter_t VNS_defaultErrorReporter;

extern VNS_cleanup_t VNS_PE_userCleanup;

/*! \brief Pointer to the error Reporter

	Once VNS_setErrorReporter() is called, PE() will then use this function pointer to call
	your specified error Reporter.
 */
extern VNS_eReporterP_t VNS_errorReporter;

char* VNS_retStr (int returnValue, int desiredRv);

#ifdef REINITIALIZE_ERRNO
#define INIT_ERRNO errno = 0
#else
// don't init errno
#define INIT_ERRNO 0
#endif

// penultimate step in expansion of PE_NOT macro
#define PE_NOT_PENULTIMATE(function, desiredRv, set, getter, initErrno) PE_NOT_ULTIMATE(function, set, initErrno, \
	(getter != (desiredRv)) ? /* was the desired value not returned? */ \
		( \
			(getter == -1) ? /* did the function indicate an error? */ \
				(VNS_errorReporter(#function, __FILE__, __func__, __LINE__, errno, NULL), -1) : /* report an error */ \
				(VNS_errorReporter(#function, __FILE__, __func__, __LINE__, 0, VNS_retStr(getter, (desiredRv))), getter) \
		) : \
		(getter) /* here or above, provide the return value */ \
)

#if defined(__GNUC__) && !defined(ALLOW_PE_AT_FILE_LEVEL)
#define USING_DECL_INSIDE_EXP

// final step in expansion of PE* macros
	// set calls the function and stores the return value
	// getter gets the return value
	// other parameters described below.
#define PE_FINAL(function, info, check_value, set, getter, error, initErrno) ({ \
	initErrno; /* zero errno if desired */ \
	__typeof__(function) set; /* declare variable for return value */ \
	(getter check_value) ? /* did an error occur */ \
		(VNS_errorReporter(#function, __FILE__, __func__, __LINE__, error, info), getter) : \
		getter; /* here or above, expression always provides the return value */ \
})

// ultimate step in expansion of PE_NOT macro
	// expression is a nested '?' expression that checks for the desired return value or whether an error was indicated
#define PE_NOT_ULTIMATE(function, set, initErrno, expression) ({ \
	initErrno; \
	__typeof__(function) set; \
    expression; \
})


#else // NOT (defined(__GNUC__) && !defined(ALLOW_PE_AT_FILE_LEVEL))
// but don't we need GNUC in order to use typeof?
// should test with compiler incompatible with GNUC, like perhaps IBM or Oracle/Sun compiler
#define PE_FINAL(function, info, check_value, set, getter, error, initErrno) ( \
	(initErrno), \
	(set), \
	(getter check_value) ? \
		(VNS_errorReporter(#function, __FILE__, __func__, __LINE__, error, info), (typeof (function)) getter) : \
		(typeof (function)) getter \
)

#define PE_NOT_ULTIMATE(function, set, initErrno, expression) ( \
	(initErrno), \
	(set), \
	expression \
)

//Function Return Value
#ifdef HAVE_TLS
	extern __thread ssize_t VNPE_non_ptr;
	extern __thread void *VNPE_ptr;
#else // ndef HAVE_TLS
#include <pthread.h>
	//extern int VNPE_non_ptr; // consider using globals for single-threaded systems
	extern pthread_key_t ReturnValue_key;

// See Macro Pthread implementation.
#define VNPE_NO_CAST

// I assume ssize_t is always at least as big as an int
// But, can ssize_t always fit into a void*?
// If not, we might have to divide an ssize_t up and store
//	it in 2 void*s.
#define VNPE_CAST (ssize_t)

#ifdef __LP64__
// to avoid warning messages we probably need
// ssize_t to translate into a long
#define VNPE_64_LC VNPE_CAST
#else



/*
 * We are on a 32 bit system
 * We need a blank define when using pthread_setspecific (VNPE_64_LC).
 * */
#define VNPE_64_LC
#endif // __LP64__
//pthread_setspecific version

#define PE_ERRNO_USED(function, info, check_value, set_cast_modifier, get_cast_modifer) \
	PE_FINAL(function, info, check_value, (pthread_setspecific( ReturnValue_key, set_cast_modifier (function))), get_cast_modifer pthread_getspecific(ReturnValue_key), errno, INIT_ERRNO)

#define PE_PTR(function, info, check_value) PE_ERRNO_USED(function, info, check_value, VNPE_NO_CAST, VNPE_NO_CAST)

#define PE_NON_PTR(function, info, check_value) PE_ERRNO_USED(function, info, check_value, (void *) VNPE_64_LC, VNPE_CAST)

#define PE_NOT_P(function, desiredRv) PE_NOT_PENULTIMATE(function, desiredRv, pthread_setspecific( ReturnValue_key, (void*) VNPE_64_LC (function)), \
		VNPE_CAST pthread_getspecific(ReturnValue_key), INIT_ERRNO)

#define PE_0_P(function) PE_FINAL(function, NULL, != 0, (pthread_setspecific( ReturnValue_key, (void*) VNPE_64_LC (function))), VNPE_CAST pthread_getspecific(ReturnValue_key), VNPE_CAST pthread_getspecific(ReturnValue_key), 0)

// On Linux/Mac OS X systems, EOK may not exist
#ifdef EOK
#define PE_EOK_P(function) PE_FINAL(function, NULL, != EOK, (pthread_setspecific( ReturnValue_key, (void*) VNPE_64_LC (function))), VNPE_CAST pthread_getspecific(ReturnValue_key), VNPE_CAST pthread_getspecific(ReturnValue_key), 0)
#endif

#endif // HAVE_TLS

#endif

#if defined(HAVE_TLS) || defined(USING_DECL_INSIDE_EXP) //--=---
//non-pthread_setspecific version

// intermediate step in expansion of PE* macros
	// error is the error number for the function call.  Might be errno or the ret_value or -ret_value, depending on the function.
	// initErrno allows errno to be reinitialized to 0 before the function call.
	// other parameters described below.
#define PE_INTERMEDIATE(function, info, check_value, ret_value, error, initErrno) PE_FINAL(function, info, check_value, ret_value = (function), ret_value, error, initErrno)

// PE_ERRNO_USED is for macros for functions where errno is used (compare with PE_EOK)
	// ret_value is the place to store the return value.
	// other parameters described below.
#define PE_ERRNO_USED(function, info, check_value, ret_value) PE_INTERMEDIATE(function, info, check_value, ret_value, errno, INIT_ERRNO)

// PE_PTR is for macros for functions that return a pointer, like malloc()
// PE_NON_PTR is for macros for functions that return something other than a pointer, perhaps a ssize_t
	// function is the function being called.
	// info is either NULL or a c string with more info
	// check_value is the test to check for an error indication in the returned value, which may be a pointer or not depending on the macro.  Eg. == (-1)
#define PE_PTR(function, info, check_value) PE_ERRNO_USED(function, info, check_value, VNPE_ptr)
#define PE_NON_PTR(function, info, check_value) PE_ERRNO_USED(function, info, check_value, VNPE_non_ptr)

// "Preformed" macros (ending in "_P").  See VNPE_disable.h and VNPE_reenable.h

// The PE_NOT macro is like PE but is intended for situations where a
// specific (desired) return value is required.
#define PE_NOT_P(function, desiredRv) PE_NOT_PENULTIMATE(function, desiredRv, VNPE_non_ptr = (function), VNPE_non_ptr, INIT_ERRNO)

// The PE_EOK macro is for wrapping functions to detect whether a function call
//	did not return 0, indicating an error.  If EOK is not returned, an error reporting
//  function is called.
#define PE_0_P(function) PE_INTERMEDIATE(function, NULL, != 0, VNPE_non_ptr, VNPE_non_ptr, errno = errno)

// On Linux/Mac OS X systems, EOK may not exist
#ifdef EOK
// The PE_EOK macro is for wrapping functions to detect whether a function call
//	did not return EOK, indicating an error.  If EOK is not returned, an error reporting
//  function is called.
#define PE_EOK_P(function) PE_INTERMEDIATE(function, NULL, != EOK, VNPE_non_ptr, VNPE_non_ptr, errno = errno)
#endif

#endif // defined(HAVE_TLS) || defined(__GNUC__)

/*
 The PE_NULL macro is for wrapping functions to detect whether a function call
 returns NULL to indicate an error.  If NULL is returned, an error reporting
 function is called.
 */
#define PE_NULL_P(function) PE_PTR(function, NULL, == NULL)

// The PE2_NULL macro is as above but also reports an additional piece of information
#define PE2_NULL_P(function, info) PE_PTR(function, info, == NULL)

/*
 The PE macro is for wrapping functions to detect whether a function call
 returns -1 to indicate an error.  If -1 is returned, an error reporting
 function is called.
*/
#define PE_P(function) PE_NON_PTR(function, NULL, == (-1))

// The PE2 macro is as above but also reports an additional piece of information
#define PE2_P(function, info) PE_NON_PTR(function, info, == (-1))

/*
 The PE_EOF macro is for wrapping functions to detect whether a function call
 returns EOF to indicate an error.  If EOF is returned, an error reporting
 function is called.
*/
// NOTE -- EOF is often -1 so PE is often the same as PE_EOF
#define PE_EOF_P(function) PE_NON_PTR(function, NULL, == EOF)

/*
 The PE_NEG macro is for wrapping functions to detect whether a function call
 returns a negative number to indicate an error.  If a negative number is returned, an error reporting
 function is called.
*/
#define PE_NEG_P(function) PE_NON_PTR(function, NULL, < 0)

#include "VNPE_reenable.h"

#ifdef __cplusplus
}
#endif

#endif /* VNPE_H_ */
