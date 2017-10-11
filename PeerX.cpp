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
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
// All TAs and the collective effort of the entire class through course piazza forum (including Dr Scratchley)
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
// File Name   : PeerX.cpp
// Version     : October 7th, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <arpa/inet.h>  // for htons() -- not available with MinGW
#include "PeerX.h"
#include "VNPE.h"
#include "myIO.h"

uint16_t my_htons(uint16_t n)
{
    unsigned char *np = (unsigned char *)&n;

    return
        ((uint16_t)np[0] << 8) |
        (uint16_t)np[1];
}

/* update CRC */
/*
The following XMODEM crc routine is taken from "rbsb.c".  Please refer to
    the source code for these programs (contained in RZSZ.ZOO) for usage.
As found in Chapter 8 of the document "ymodem.txt".
    Original 1/13/85 by John Byrns
    */

/*
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

unsigned short
updcrc(register int c, register unsigned crc)
{
	register int count;

	for (count=8; --count>=0;) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
			crc ^= 0x1021;
		}
		else {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
		}
	}
	return crc;
}

// returns a crc16 in 'network byte order'
// derived from code in "rbsb.c" (see above)
// line comments in function below show lines removed from original code
void
crc16ns (uint16_t* crc16nsP, uint8_t* buf)
{
	 register int wcj;
	 register uint8_t *cp;
	 unsigned oldcrc=0;
	 for (wcj=CHUNK_SZ,cp=buf; --wcj>=0; ) {
		 //sendline(*cp);

		 /* note the octal number in the line below */
		 oldcrc=updcrc((0377& *cp++), oldcrc);

		 //checksum += *cp++;
	 }
	 //if (Crcflg) {
		 oldcrc=updcrc(0,updcrc(0,oldcrc));

		 //uint16_t crc16 = oldcrc;
		 //*crc16nsP = (crc16 >> 8) + (crc16 << 8);

		 *crc16nsP = my_htons((uint16_t) oldcrc); // new
		 /* *crc16nsP = htons((uint16_t) oldcrc); // new */

		 //sendline((int)oldcrc>>8);
		 //sendline((int)oldcrc);
	 //}
	 //else
		 //sendline(checksum);
}

// returns a crc16 in 'network byte order'
// derived from code in "rbsb.c" (see above)
// line comments in function below show lines removed from original code
void
chksum8ns (uint8_t* chksum8nsP, uint8_t* buf)
{
	 register int wcj;
	 register uint8_t *cp;
	 uint8_t checksum=0;
	 for (wcj=CHUNK_SZ,cp=buf; --wcj>=0; cp++) {
		 checksum += *cp;
	 }
	 *chksum8nsP = checksum; // new
}



PeerX::
PeerX(int d, const char *fname, bool useCrc)
:result("ResultNotSet"), errCnt(0), mediumD(d), fileName(fname), transferringFileD(-1), Crcflg(useCrc)
{
}

//Send a byte to the remote peer across the medium
void
PeerX::
sendByte(uint8_t byte)
{
	PE_NOT(myWrite(mediumD, &byte, sizeof(byte)), sizeof(byte));
}

