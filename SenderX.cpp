//============================================================================
//
//% Student Name 1: student1
//% Student 1 #: 123456781
//% Student 1 userid (email): stu1 (stu1@sfu.ca)
//
//% Student Name 2: student2
//% Student 2 #: 123456782
//% Student 2 userid (email): stu2 (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : SenderX.cpp
// Version     : September 3rd, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <iostream>
#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <fcntl.h>	// for O_RDWR

#include "myIO.h"
#include "SenderX.h"
#include "VNPE.h"

using namespace std;

SenderX::SenderX(const char *fname, int d)
:PeerX(d, fname),
 bytesRd(-1),
 firstCrcBlk(true),
 blkNum(0)  	// but first block sent will be block #1, not #0
{
}

//-----------------------------------------------------------------------------

// get rid of any characters that may have arrived from the medium.
void SenderX::dumpGlitches()
{
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.

//uint8_t SenderX::sendMostBlk(blkT blkBuf)
uint8_t SenderX::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	int mostBlockSize = (this->Crcflg ? BLK_SZ_CRC : BLK_SZ_CS) - 1;
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte to the receiver
void
SenderX::
sendLastByte(uint8_t lastByte)
{
	PE(myTcdrain(mediumD)); // wait for previous part of block to be transmitted to receiver
	dumpGlitches();			// dump any received glitches

	PE_NOT(myWrite(mediumD, &lastByte, sizeof(lastByte)), sizeof(lastByte));
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
//void SenderX::genBlk(blkT blkBuf)
void SenderX::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(read(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if(bytesRd>0)
	{
		blkBuf[0] = SOH;
		uint8_t nextBlkNum = blkNum + 1;
		blkBuf[SOH_OH] = nextBlkNum;
		blkBuf[SOH_OH + 1] = ~nextBlkNum;
		if (this->Crcflg) {
			/*add padding*/
			if(bytesRd<CHUNK_SZ)
			{
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
			}

			/* calculate and add CRC in network byte order */
			crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
		}
		else {
			//checksum
			blkBuf[PAST_CHUNK] = blkBuf[DATA_POS];
			for( int ii=DATA_POS + 1; ii < DATA_POS+bytesRd; ii++ )
				blkBuf[PAST_CHUNK] += blkBuf[ii];

			//padding
			if( bytesRd < CHUNK_SZ )  { // this line could be deleted
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
				blkBuf[PAST_CHUNK] += CTRL_Z * padSize;
			}
		}
	}
}

/* tries to prepare the first block.
*/
void SenderX::prep1stBlk()
{
	// **** this function will need to be modified ****
	genBlk(blkBufs[0]);
}


void SenderX::cs1stBlk()
{
	// **** this function will need to be modified ****
	Crcflg = false;
	genBlk(blkBufs[1]);
}

/* while sending the now current block for the first time, prepare the next block if possible.

	Note: Blk # blkBufs[0] blkBufs[1] ACKNAK
			1		0			1		ACK
			2		1			0		NAK
			3		0			1		ACK

*/
void SenderX::sendBlkPrepNext()
{
	// **** this function will need to be modified ****
	blkNum ++; // 1st block about to be sent or previous block ACK'd
	uint8_t lastByte;
	if(blkNum % 2) {
		// Even
		lastByte = sendMostBlk(blkBufs[1]);
		genBlk(blkBuf[0]); // prepare next block
	} else {
		// Odd
		lastByte = sendMostBlk(blkBufs[0]);
		genBlk(blkBufs[1]); // prepare next block
	}
	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the xmodem receiver
void SenderX::resendBlk()
{
	// resend the block including the checksum
	//  ***** You will have to write this simple function *****
	if(blkNum % 2) {
		// Even
		sendMostBlk(blkBufs[1]);
	} else {
		// Odd
		sendMostBlk(blkBufs[0]);
	}
}

//Send 8 CAN characters in a row (in pairs spaced in time) to the
//  XMODEM receiver, to inform it of the canceling of a file transfer
void SenderX::can8()
{
	// is it wise to dump glitches before sending CANs?
	//dumpGlitches();

	const int canLen=2;
    char buffer[canLen];
    memset( buffer, CAN, canLen);

	const int canPairs=4;
	/*
	for (int x=0; x<canPairs; x++) {
		PE_NOT(myWrite(mediumD, buffer, canLen), canLen);
		usleep((TM_2CHAR + TM_CHAR)*1000*mSECS_PER_UNIT/2); // is this needed for the last time through loop?
	};
	*/
	int x = 1;
	while (PE_NOT(myWrite(mediumD, buffer, canLen), canLen),
			x<canPairs) {
		++x;
		usleep((TM_2CHAR + TM_CHAR)*1000*mSECS_PER_UNIT/2);
	}
}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDONLY, 0);
	if(transferringFileD == -1) {
		cout /* cerr */ << "Error opening input file named: " << fileName << endl;
		result = "OpenError";
	}
	else {
		//blkNum = 0; // but first block sent will be block #1, not #0
		prep1stBlk();

		// ***** modify the below code according to the protocol *****
		// below is just a starting point.  You can follow a
		// 	different structure if you want.
		char byteToReceive;
		PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a 'C'
		Crcflg = true;
		
		enum state { START, ACKNAK, EOT1, EOTEOT, CAN};
		bool done = false;
		state = START;
		
		while (!done) {
			try {
			switch (state){
				case START:{
					if(bytesRd)
					{
						if( byteToReceive == 'C' || 
							byteToReceive == NAK ){
							if(byteToReceive == NAK){
								Crcflg = false;
								cs1stBlk();
								firstCrcBlk = false;
								sendBlkPrepNext();
							}
							state = ACKNAK;
						}
					} else { // file is empty
						if( byteToReceive == 'C' || 
							byteToReceive == NAK ){
							if(byteToReceive == NAK){
								firstCrcBlk = false;
							}
							sendByte(EOT); // send the first EOT
							state = EOT1;
						}
					}
					// TODO: do we throw exception at START state?
				}
				case ACKNAK: {
					if(bytesRd){
						if( byteToReceive == ACK ) {
							sendBlkPrepNext();
							errCnt = 0;
							firstCrcBlk = false;
							state = ACKNAK;
						}
						else if( byteToReceive == NAK ||
							 (byteToReceive == 'C' && firstCrcBlk ) &&
							 (errCnt <= errB) ) // errB = 10, perform action at 11 times
						{
							resendBlk();
							errCnt++;
						} else if (byteToReceive == NAK && (errCnt >= errB)) {
							can8();
							result = "ExcessiveNAKs";
							done = true;
						}
						} else if ( byteToReceive == CAN)
							state = CAN;
					}
					else { // !bytesRd
						if(byteToReceive == ACK) {
							sendByte(EOT);
							errCnt = 0;
							firstCrcBlk = false;
							state = EOT1;
						}
					}
					// TODO: when to throw exception at ACKNAK state?
				}
				case EOT1: {
					if(byteToReceive == NAK){
						sendByte(EOT);
						state = EOTEOT;
					} else if(byteToReceive == ACK) {
						result = "1st EOT ACK'd";
						done = true;
					} else {
						throw; // unexpected char received
					}	
				}
				case EOTEOT: {
					if(byteToReceive == ACK) {
						result = "Done";
						done = true;
					} else {
						throw; // unexpected char received (NAK or other char)
					}
				}
				case CAN: {
					if(byteToReceive == CAN) {
						result = "RcvCancelled";
						done = true;
					} else
						throw; // unexpected char received. Expect: CAN
				}
				 
					// unknown state
				
			}
			//sendBlkPrepNext();
			// assuming below we get an ACK
			PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
			}
			catch(...)
			{
				cerr << "Sender received totally unexpected char #" << byteToReceive << ": " << \
				(char)byteToReceive << endl;
				PE(myClose(transferringFileD));
				exit(EXIT_FAILURE);
			}
		}
		// ========================== Craig's Code ==========================
		
		//sendByte(EOT); // send the first EOT
		//PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a NAK
		//sendByte(EOT); // send the second EOT
		//PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get an ACK

		PE(myClose(transferringFileD));
		/*
		if (-1 == myClose(transferringFileD))
			VNS_ErrorPrinter("myClose(transferringFileD)", __func__, __FILE__, __LINE__, errno);
		*/
		result = "Done";  // should this be moved above somewhere??
		
		// ========================== Craig's Code ==========================
	}
}

