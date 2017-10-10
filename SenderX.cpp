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
// Resources: ENSC 351 forum on piazza.com
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
// Version     : October 6th, 2017
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

//#define _DEBUG

#define ASCII(argStr) testASCII(argStr)

string testASCII(int argStr) {
	switch (argStr) {
	case 1: // SOH
		return "SOH";
		break;
	case 4: // EOT
		return "EOT";
		break;
	case 6: //ACK
		return "ACK";
		break;
	case 21: // NAK
		return "NAK";
		break;
	case 24: //CAN
		return "CAN";
		break;
	default:
		return "Unknown ASCII";
		break;
	}
}

SenderX::SenderX(const char *fname, int d) :
		PeerX(d, fname), bytesRd(-1), firstCrcBlk(true), blkNum(0) // but first block sent will be block #1, not #0
{
}

//-----------------------------------------------------------------------------

// get rid of any characters that may have arrived from the medium.
void SenderX::dumpGlitches() {
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz
			== (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))))
		;
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.

//uint8_t SenderX::sendMostBlk(blkT blkBuf)
uint8_t SenderX::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC]) {
	int mostBlockSize = (this->Crcflg ? BLK_SZ_CRC : BLK_SZ_CS) - 1;
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte to the receiver
void SenderX::sendLastByte(uint8_t lastByte) {
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
void SenderX::genBlk(uint8_t blkBuf[BLK_SZ_CRC]) {
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(read(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if (bytesRd > 0) {
		blkBuf[0] = SOH;
		uint8_t nextBlkNum = blkNum + 1;
		blkBuf[SOH_OH] = nextBlkNum;
		blkBuf[SOH_OH + 1] = ~nextBlkNum;
		/*add padding*/
		if (bytesRd < CHUNK_SZ) {
			//pad ctrl-z for the last block
			uint8_t padSize = CHUNK_SZ - bytesRd;
			memset(blkBuf + DATA_POS + bytesRd, CTRL_Z, padSize);
		}
		if (this->Crcflg)
			/* calculate and add CRC in network byte order */
			crc16ns((uint16_t*) &blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
		else
			chksum8ns(&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
	}
}

/* tries to prepare the first block.
 */
void SenderX::prep1stBlk() {
	// **** this function will need to be modified ****
	genBlk(blkBufs[0]);
}

void SenderX::cs1stBlk() {
	// **** this function will need to be modified ****
	Crcflg = false;
	chksum8ns(&blkBufs[0][PAST_CHUNK], &blkBufs[0][DATA_POS]);
}

/* while sending the now current block for the first time, prepare the next block if possible.

 Note: Blk # blkBufs[0] blkBufs[1] ACKNAK
 1		0			1		ACK
 2		1			0		NAK
 3		0			1		ACK

 */
void SenderX::sendBlkPrepNext() {
	// **** this function will need to be modified ****
	blkNum++; // 1st block about to be sent or previous block ACK'd
	uint8_t lastByte;

	if (blkNum % 2) {
		// Odd
		lastByte = sendMostBlk(blkBufs[0]);
		genBlk(blkBufs[1]); // prepare next block
	} else {
		// Even
#ifdef _DEBUG
		// ###TEST data corruption (data flow 1)
		//if(blkNum == 2) {
		//	memcpy(blkBuf, blkBufs[1],BLK_SZ_CRC);
		//	blkBuf[DATA_POS+1] = 0;
		lastByte = sendMostBlk(blkBuf);
		//}
		//else
#endif
		lastByte = sendMostBlk(blkBufs[1]);
		genBlk(blkBufs[0]); // prepare next block
	}
	sendLastByte(lastByte);

	// ------ craig's code -------
//	blkNum++; // 1st block about to be sent or previous block ACK'd
//	uint8_t lastByte = sendMostBlk(blkBuf);
//	genBlk(blkBuf); // prepare next block
//	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the xmodem receiver
void SenderX::resendBlk() {
//	 resend the block including the checksum
//	  ***** You will have to write this simple function *****
	uint8_t lastByte;
	if (blkNum % 2) {
		// Odd
		lastByte = sendMostBlk(blkBufs[0]);
	} else {
		// Even
		lastByte = sendMostBlk(blkBufs[1]);
	}
	sendLastByte(lastByte);
}

//Send 8 CAN characters in a row (in pairs spaced in time) to the
//  XMODEM receiver, to inform it of the canceling of a file transfer
void SenderX::can8() {
	// is it wise to dump glitches before sending CANs?
	//dumpGlitches();

	const int canLen = 2;
	char buffer[canLen];
	memset(buffer, CAN, canLen);

	const int canPairs = 4;
	/*
	 for (int x=0; x<canPairs; x++) {
	 PE_NOT(myWrite(mediumD, buffer, canLen), canLen);
	 usleep((TM_2CHAR + TM_CHAR)*1000*mSECS_PER_UNIT/2); // is this needed for the last time through loop?
	 };
	 */
	int x = 1;
	while (PE_NOT(myWrite(mediumD, buffer, canLen), canLen), x < canPairs) {
		++x;
		usleep((TM_2CHAR + TM_CHAR) * 1000 * mSECS_PER_UNIT / 2);
	}
}

void SenderX::sendFile() {
	transferringFileD = myOpen(fileName, O_RDONLY, 0);
	if (transferringFileD == -1) {
		cout /* cerr */<< "Error opening input file named: " << fileName
				<< endl;
		result = "OpenError";
	} else {

		//blkNum = 0; // but first block sent will be block #1, not #0
		prep1stBlk();

		// ***** modify the below code according to the protocol *****
		// below is just a starting point.  You can follow a
		// 	different structure if you want.
		char byteToReceive;
//		PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a 'C'
		Crcflg = true;

		enum State {
			START, ACKNAK, EOT1, EOTEOT, CANC
		};
		bool done = false;
		State state = START;

		while (!done) {
			PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
			try {
				switch (state) {
				case START: {
#ifdef _DEBUG
					cout << "Sender: ";
					cout << "START" << endl;
#endif
					// DONE: do we throw exception at START state? Ans: Yes, according to the sender state diagram.
					if (!(byteToReceive == 'C' || byteToReceive == NAK))
						throw 0;

					if (bytesRd) {
						if (byteToReceive == NAK) {
							Crcflg = false;
							cs1stBlk();	// replace blkbufs[0] last byte to checksum
							firstCrcBlk = false;
						}
						sendBlkPrepNext();
						state = ACKNAK;
					} else { // file is empty/OPEN_ERROR

						if (byteToReceive == NAK) {
							firstCrcBlk = false;
						}
						sendByte(EOT); // send the first EOT
						state = EOT1;
					}
					break;

				}
				case ACKNAK: {
#ifdef _DEBUG
					cout << "Sender: ";
					cout << "@ACKNAK" << " received " << ASCII((int)byteToReceive) << endl;
#endif
					if (bytesRd > 0 && byteToReceive == ACK) {
						sendBlkPrepNext();
//							// ###TESTING
//							sendByte(CAN);

						errCnt = 0;
						firstCrcBlk = false;
						state = ACKNAK;
					} else if ((byteToReceive == NAK
							|| (byteToReceive == 'C' && firstCrcBlk))
							&& (errCnt <= errB)) // errB = 10, perform action at 11 times
							{
						resendBlk();
						errCnt++;
						state = ACKNAK;
#ifdef _DEBUG
						cout << "Sender: ";
						cout << "@ACKNAK" << " errCnt " << errCnt << endl;
#endif
					} else if (byteToReceive == NAK && (errCnt > errB)) {
						can8();
						result = "ExcessiveNAKs";
						done = true;
					} else if (byteToReceive == CAN) {
						/* if the CAN that sender just received was due to corruption (from ACK or NAK),
						 * receiver will not proceed until hearing another msg from sender.
						 * Thus we resend last blk to unblock rcver in this scenario.
						 * Note if the rcver is in the middle of sending 8 bytes of CAN, this wouldn't affect anything
						 */
						//resendBlk();
						// Resending blk would prevent Rcver from hanging in the case of ACK/NAK corrupted to CAN
//						// However for the scope of this assignment, this is a scenario that would not occur in the kind medium provided in Part 2
						state = CANC;
					} else if (!bytesRd && byteToReceive == ACK) {
						sendByte(EOT);
						errCnt = 0;
						firstCrcBlk = false;
#ifdef _DEBUG
						cout << "Sender: ";
						cout << "@ACKNAK" << " errCnt " << errCnt << endl;
#endif
						state = EOT1;
					} else
						throw 0;

					break;
					// DONE: when to throw exception at ACKNAK state? ANS: when its a not covered char, already covered
				}
				case EOT1: {
#ifdef _DEBUG
					cout << "Sender: ";
					cout << "@EOT1" << " received " << ASCII((int)byteToReceive) << endl;
#endif
					if (byteToReceive == NAK) {
						sendByte(EOT);
						state = EOTEOT;
					} else if (byteToReceive == ACK) {
						result = "1st EOT ACK'd";
						done = true;
					} else {
						throw 0; // CAN or unexpected char received
					}
					break;
				}
				case EOTEOT: {
#ifdef _DEBUG
					cout << "Sender: ";
					cout << "@EOTEOT" << " received " << ASCII((int)byteToReceive) << endl;
#endif
					if (byteToReceive == ACK) {
						result = "Done";
						done = true;
					} else if (byteToReceive == NAK) {
						/* When the rcver NAKs twice in a row:
						 * 1st EOT was rcved -> NAK -> second one corrupted -> Sender should resend 2nd EOT
						 * 1st EOT was corrupted as well -> NAK(potentially rcver updated errCnt) -> Sender should resend 2nd EOT and acknowledge this corruption, by errCnt++ (and check if exceeded errB)
						 * 						(if no more corruption: rcver should reply another NAK -> another resend in EOTEOT -> reply with ACK -> done)
						 *
						 */
						if (++errCnt >= errB) {
							can8();
							result = "ExcessiveNAKs";
							done = true;
							break;
						}
						sendByte(EOT);
						state = EOTEOT;
					} else if (byteToReceive == CAN) {
						/* Note: jumping to CANC state is perfectly fine.
						 * EVEN if this CAN is due to corruption, we will go thru the whole EOT1 -> EOTEOT again, when we recover from CANC state back to ACKNAK
						 */
						state = CANC;
					} else
						throw 0; // unexpected char received (NAK or other char)

					break;
				}
				case CANC: {
#ifdef _DEBUG
					cout << "Sender: ";
					cout << "@CANC" << " received " << ASCII((int)byteToReceive) << endl;
#endif
					if (byteToReceive == CAN) {
						result = "RcvCancelled";
						done = true;
#ifdef _DEBUG
						cout << "Sender: ";
						cout << "DONE" << endl;
#endif
					} else if (byteToReceive == NAK || byteToReceive == ACK) {
						// the last CAN that the sender rcved was due to error, update errCnt and check if > errB
						// then recover to ACKNAK state
						if (++errCnt >= errB) {
							can8();
							result = "ExcessiveNAKs";
							done = true;
							break;
						}
						if (byteToReceive == NAK)
							resendBlk();
						else
							sendBlkPrepNext();
						state = ACKNAK;
					} else
						throw 0; // unexpected char received
					break;
				}
				}

			} catch (...) {
				cerr << "Sender received totally unexpected char #"
						<< (int) byteToReceive << ": "
						<< ASCII((int )byteToReceive) << endl;
				PE(myClose(transferringFileD));
				exit (EXIT_FAILURE);
			}
		}
		// ========================== Craig's Code ==========================
//		while (bytesRd) {
//			sendBlkPrepNext();
//			// assuming below we get an ACK
//			PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
//		}
//		sendByte(EOT); // send the first EOT
//		PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a NAK
//		sendByte(EOT); // send the second EOT
//		PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get an ACK

		PE(myClose(transferringFileD));
		/*
		 if (-1 == myClose(transferringFileD))
		 VNS_ErrorPrinter("myClose(transferringFileD)", __func__, __FILE__, __LINE__, errno);
		 */
//		result = "Done";  // should this be moved above somewhere??
		// ========================== Craig's Code ==========================
	}
}

