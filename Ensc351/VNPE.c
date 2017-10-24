/*
 VNS_PE.c is part of the VectorNav Support Library http://vnsupport.sourceforge.net/

		Copyright (C) 2009 - 2010 Craig Scratchley (craig_scratchley@alumni.sfu.ca),
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

#include "VNPE.h"
//#include <syslog.h>
#include <string.h> // for strerror() (and strlen() ?)
#include <stdio.h>
#include <stdlib.h> // for exit(), malloc(), free()
#include "AtomicConsole.h"

// as recommended at:  http://gcc.gnu.org/onlinedocs/gcc-4.4.7/gcc/Function-Names.html
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

// consider GNU extension:  program_invocation_name
extern char * __progname;

void halt(void)
{ // always have a breakpoint set here to allow the halting
} // use gdbinit file (alternative to .gdbinit)

void haltExit(int returnCode)
{
	halt();
	sleep(1);
	exit(returnCode);
}

char* VNS_retStr (int returnValue, int desiredRv)
{
	const char* format = "Return of %d instead of %d";
	char* string;
	// use malloc because we need string to be available after function returns but can't use a global.
	if ((string = malloc(strlen(format)+ 1 + 2*10)) == NULL) {
		VNS_ErrorPrinter ("malloc(strlen(format) + 1 + 2*10)", __FILE__, __func__, __LINE__, errno, NULL);
		string = "Unexpected return value";
	}
	else {
		if (sprintf(string, format, returnValue, desiredRv) < 0) {
			VNS_ErrorPrinter ("sprintf(string, format, returnValue, desiredRv)", __FILE__, __func__, __LINE__, errno, NULL);
			string = "Unexpected return value";
		}
	}
	return string;
}

void VNS_ErrorPrinter
(const char* functionCall, const char* file, const char *func, int line, int error, const char* info)
{
//	const char *func = "funcPlaceholder";
	const char* prefix;
	const char* midfix;
	const char* funcStr;
	size_t funcLen;
	char* midBuffer = NULL;
	char* funcBuffer = NULL;
#define STRERROR_LEN BUFSIZ
#define PREBUFFER_LEN (STRERROR_LEN + 26)
	char preBuffer[PREBUFFER_LEN];
	if (!error) {
		if (info) {
			// desired return value was not returned
			prefix = info;
			midfix = "";
		}
		else {
			prefix = "*** Error occurred with unexpected error number of 0";
			midfix = "";
		}
	}
	else {
		char strerror_buff[STRERROR_LEN];
		int strerror_error;
		if ((strerror_error = strerror_r ( error, strerror_buff, STRERROR_LEN)) > 0)
			VNS_ErrorPrinter ("sstrerror_r ( error, strerror_buff, STRERROR_LEN))", __FILE__, __func__, __LINE__, strerror_error, NULL); // provide error as info?
		if (sprintf (preBuffer, "*** Error %d (%s) occurred", error, strerror_buff) < 0) {
			VNS_ErrorPrinter ("sprintf (preBuffer, \"*** Error %d (%s) occurred\", error, strerror(error))", __FILE__, __func__, __LINE__, errno, strerror_buff);
			prefix = strerror_buff;
		}
		else
			prefix = preBuffer;
		if (info) {
			const char* format = "\t additional info: %s\n";
			if ((midBuffer = malloc(strlen(format) + strlen(info))) == NULL) {
				VNS_ErrorPrinter ("malloc(strlen(format) + strlen(info))", __FILE__, __func__, __LINE__ - 1, errno, NULL); // provide malloc size as info?
				midfix = info;
			}
			else {
				if (sprintf (midBuffer, format, info) < 0) {
					VNS_ErrorPrinter ("sprintf (midBuffer, format, info)", __FILE__, __func__, __LINE__ - 1, errno, info);
					midfix = info;
				}
				else
					midfix = midBuffer;
			}
		}
		else
			midfix = "";
	}
	funcLen = strlen(func);
	if (funcLen) {
		const char* format = "in function '%s'";
		if ((funcBuffer = malloc(strlen(format) + funcLen)) == NULL) {
			VNS_ErrorPrinter ("malloc(strlen(format) + funcLen)", __FILE__, __func__, __LINE__ - 1, errno, NULL); // provide malloc size as info?
			funcStr = func;
		}
		else {
			if (sprintf (funcBuffer, format, func) < 0) {
				VNS_ErrorPrinter ("sprintf (funcBuffer, format, func)", __FILE__, __func__, __LINE__ - 1, errno, info);
				funcStr = func;
			}
			else
				funcStr = funcBuffer;
		}
	}
	else
		funcStr = "at the file level";

	pthread_mutex_lock(&consoleMutex);
	// check for fprintf failure?
	fprintf(stderr,"\n%s \n"
			"\t in executable '%s' \n"
			"\t %s at line %d of file '%s'\n"
			"\t resulting from the invocation: %s\n"
			"%s",
			prefix, __progname, funcStr, line, file, functionCall, midfix);
	pthread_mutex_unlock(&consoleMutex);

	// configure syslog here
	/*
	syslog(LOG_USER | LOG_ERR, // VNS_PRIORITY_ERROR,
			"%s at line %d of file %s %sfrom: %s \n",
			prefix, line, file, midfix, functionCall);
			*/
	// if (midBuffer) // apparently not needed
		free(midBuffer);
	free(funcBuffer);
	//if (error == EOK) // for malloc in VNS_retStr
	if (error == 0) // for malloc in VNS_retStr
		free((char*) info);

	errno = error; // ensure errno wasn't changed by functions called.
}

VNS_cleanup_t VNS_PE_userCleanup = NULL;

//Default error Reporter. Replace with something more suitable to you if desired.
void VNS_defaultErrorReporter (const char* functionCall, const char* file, const char *func, int line, int error, const char* info)
{
	VNS_ErrorPrinter (functionCall, file, func, line, error, info);

	// is user cleanup necessary?  atexit allows functions to be called by the exit function.
	// do we need to be able to have a list of cleanup functions?
	if (VNS_PE_userCleanup) {
		VNS_PE_userCleanup();
	};
    pthread_mutex_lock(&consoleMutex);
    // check for fprintf failure?
	fprintf (stderr, "\t ErrorReporter in VNPE.c exiting process!\n");
    pthread_mutex_unlock(&consoleMutex);

	/*
	syslog(LOG_USER | LOG_ERR, // VNS_PRIORITY_ERROR,
			"VNPE.c ErrorReporter exiting process!\n");
			*/
	// if called in C++ program, perhaps should throw a C++ exception.
	haltExit(EXIT_FAILURE);
	// already exited, so don't need to do the lines below.
}

VNS_eReporterP_t VNS_errorReporter = &VNS_defaultErrorReporter;

// (gcc) requires a return value to use the function at the file level.
int VNS_setErrorReporter(VNS_eReporter_t eReporterFn)
{
	if(eReporterFn == NULL)
		VNS_errorReporter = &VNS_defaultErrorReporter;
	else
		VNS_errorReporter = eReporterFn;
	return 0;
}

#ifdef HAVE_TLS
__thread ssize_t VNPE_non_ptr = 0;
__thread void *VNPE_ptr = NULL;
#else
//int VNPE_non_ptr = 0; // use global for single-threaded systems
pthread_key_t ReturnValue_key;

// This works with GNU gcc -- is it portable to all compilers?
/* ^ Definitely not. __attribute__ is a gcc construct. Most systems seem to support some form for thread-local storage.
 We can add defines to differentiate between different compilers. however MinGW and linux gcc should support tlc.
 This code should remain for Mac OS X however.
 */
void __attribute__ ((constructor)) VNS_PE_init(void)
{
	int returnValue;
	if ((returnValue = pthread_key_create( &ReturnValue_key, NULL)) != 0)
		VNS_defaultErrorReporter ("pthread_key_create", __FILE__, __func__, __LINE__, returnValue, NULL);

	// test out pthread_setspecific -- there could possibly be a memory or other problem.
	if ((returnValue = pthread_setspecific( ReturnValue_key, (void*) 1)) != 0)
		VNS_defaultErrorReporter ("pthread_setspecific", __FILE__, __func__, __LINE__, returnValue, NULL);
}
#endif
