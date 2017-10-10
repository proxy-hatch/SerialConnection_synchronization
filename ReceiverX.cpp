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
// File Name   : ReceiverX.cpp
// Version     : October 7th, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"

using namespace std;

//#define _DEBUG

#define ASCII(argStr) test_ASCII(argStr)

string test_ASCII(int argStr) {
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

ReceiverX::ReceiverX(int d, const char *fname, bool useCrc) :
		PeerX(d, fname, useCrc), goodBlk(false), goodBlk1st(false), numLastGoodBlk(
				0) {
	Crcflg = useCrc;
	NCGbyte = useCrc ? 'C' : NAK;
}

void ReceiverX::receiveFile() {

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	// should we test for error and set result to "OpenError" if necessary?
	transferringFileD = PE2(creat(fileName, mode), fileName);

	// ***** improve this member function *****
	// below is just an example template.  You can follow a
	// 	different structure if you want.
	// inform sender that the receiver is ready and that the
	//		sender can send the first block
	sendByte(NCGbyte); // 'C' (CRC case) by default

	enum State {
		SOHEOT, // also represent START state
		EOTEOT,
		CANC,
		TRAN // Transition State
	};
	// initialize variables
	State state = SOHEOT;
	State prevState = SOHEOT;
	bool done = false;
	errCnt = 0; // variable not initialized in constructor
//	// ###TESTING
//	errCnt=5;

	while (!done) {
		// TODO: move 'PE_NOT(myRead(mediumD, rcvBlk, 1), 1);' outside of state to avoid repetition/confusion
		// only make this change after part 2, if its still needed
		try {
			switch (state) {
			case SOHEOT: {
				if(prevState != CANC && prevState != EOTEOT)	// if coming from CANC or EOTEOT, do not update rcvBlk again
					PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
#ifdef _DEBUG
				cout << "Receiver: ";
				cout << "@SOHEOT" << " received " << ASCII((int)rcvBlk[0]) << " <- "
				<< (int) rcvBlk[1] << endl;
#endif

				if (rcvBlk[0] == SOH) {
					getRestBlk();
					state = TRAN;
					prevState = SOHEOT;
				} else if (rcvBlk[0] == EOT) {
					sendByte(NAK);
					state = EOTEOT;
					prevState = SOHEOT;
				} else if (rcvBlk[0] == CAN) {
					// rcver NAKs this FIRST CAN, in case its a result of corruption
					/* scenarios: (sender actual) -> (behaviour due to this NAK)
					 * SOH -> resend
					 * EOT -> continue to send second EOT
					 * 			(rcver will then hang as it expects another EOT, but nothing we can do until implement timeout)
					 * CAN -> no problem, sender does not even read this NAK, continues to send CANs
					 */
					sendByte(NAK);
					state = CANC;
					prevState = SOHEOT;
				} else
					throw 0;
				break;
			}
			case TRAN: {
#ifdef _DEBUG
				//cout << "Receiver: ";
				//cout << "@TRAN" << endl;
#endif
				if (errCnt <= errB) {// Number of consecutive NAKs still within error Boundary

					if (goodBlk) {
#ifdef _DEBUG
						//cout << "Receiver: ";
						//cout << "sends ACK" << endl;
						cout << endl;
#endif
						sendByte(ACK);
//						// ###TESTING
//						if(errCnt==5){
//							sendByte(CAN);
//							errCnt=0;
//						}else
//							sendByte(ACK);


						if (goodBlk1st){
							// reset errCnt and output data
							errCnt = 0;
							writeChunk();
							numLastGoodBlk = rcvBlk[1];
						}
						else
							errCnt++;
					} else {
#ifdef _DEBUG
						//cout << "Receiver: ";
						//cout << "sends NAK" << endl;
						cout << endl;
#endif
						sendByte(NAK);
						errCnt++;
					}
					state = SOHEOT;
					prevState = TRAN;
				} else {// errCnt > errBoundary, initiate terminate by sending CAN8
					can8();

					if (errCnt == errB + 2)
						result = "RcvCancelled because of unexpected blkNum";
					else
						result = "RcvCancelled because of exceeded errB";
					done = true;
				}
				break;
			}
			case EOTEOT: {
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
#ifdef _DEBUG
				cout << "Receiver: ";
				cout << "@SOHEOT" << " received " << ASCII((int)rcvBlk[0]) << endl;
#endif

				if (rcvBlk[0] == EOT) {
					sendByte(ACK);
					result = "Done";
					done = true;
				}
				else if (rcvBlk[0] == SOH || rcvBlk[0] == EOT) {
					state = SOHEOT;
					prevState = EOTEOT;
				} else
					throw 0;
				break;
			}
			case CANC: {
				// read the second blk after the first blk[0]=='CAN' occurence
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
#ifdef _DEBUG
				cout << "Receiver: ";
				cout << "@CAN" << " received " << ASCII((int)rcvBlk[0]) << endl;
#endif
				if (rcvBlk[0] == CAN) {		// second CAN blk in a row
					result = "SndCancelled";
					done = true;
				}
				// Ques: Can't go back to SOHEOT from CANC state (bytes got corrupted)
				// What happens if don't get another cancel in CANC state
				// ANS: You CAN and SHOULD go back to SOHEOT from CANC if you dont rcv a second CAN
				else if (rcvBlk[0] == SOH || rcvBlk[0] == EOT) {
					state = SOHEOT;
					prevState = CANC;
				} else
					throw 0;
				break;
			}
			}

		} catch (...) {
			cerr << "Receiver received totally unexpected char #"
					<< (int) rcvBlk[0] << ": " << ASCII((int )rcvBlk[0])
					<< endl;
			PE(myClose(transferringFileD));
			exit (EXIT_FAILURE);
		}

	};
	// EOT was presumably just read in the condition for the while loop
	//sendByte(NAK); // NAK the first EOT
	//PE_NOT(myRead(mediumD, rcvBlk, 1), 1); // presumably read in another EOT
	//sendByte(ACK); // ACK the second EOT
	PE(close(transferringFileD));
	//result = "Done"; // move this line above somewhere?
}

/* Only called after an SOH character has been received.
 The function tries
 to receive the remaining characters to form a complete
 block.  The member variable goodBlk will be made false if
 the received block formed has something
 wrong with it, like the checksum being incorrect.  The member
 variable goodBlk1st will be made true if this is the first
 time that the block was received in "good" condition.
 */
void ReceiverX::getRestBlk() {
	// ********* this function must be improved ***********
	PE_NOT(
			myReadcond(mediumD, &rcvBlk[1], Crcflg==true?REST_BLK_SZ_CRC:REST_BLK_SZ_CS, Crcflg==true?REST_BLK_SZ_CRC:REST_BLK_SZ_CS, 0, 0),
			Crcflg==true?REST_BLK_SZ_CRC:REST_BLK_SZ_CS);

	// ###TEST: intentionally scramble blkNum complement
	//if(rcvBlk[1]>0)
	//	rcvBlk[2]=rcvBlk[2]-1;

	goodBlk1st = goodBlk = true; // True by default. Only set to false when conditions are not met

	// Block number and the 1's complement should
	// add up to 255, if not, NAK the block
	if (rcvBlk[1] + rcvBlk[2] != 255) {
		//sendByte(NAK);
		goodBlk = false;
		goodBlk1st = false;
		return;
	}

	// Ensure blkNum sent is either the block number expected or
	// the number of the error-free block previously received
	// If not, cancel the transfer by sending 8 CAN bytes.
	// i.e. last block is 2 but this blknum is 4
	/* Corner case. */
	uint8_t currBlkNum = numLastGoodBlk + 1;		// expected current blkNum
	if (currBlkNum == 256)	// wrap around blkNum
		currBlkNum = 0;

	if (rcvBlk[1] != numLastGoodBlk && rcvBlk[1] != currBlkNum) {
#ifdef _DEBUG
		cout << endl << "Receiver: sends abort. " << endl;
#endif
		/* Since the rcver should always terminate and send CAN8 the moment it exceeds errB (errCnt = errB + 1),
		 * we can use errB + 2 as a special case:
		 * 		"blkNum exceeds the allowed window (eceptedBlkNum || eceptedBlkNum -1)"
		 * It should then terminate with CAN8 as usual.
		 */
		errCnt = errB + 2;
		return;
	}

	// Prevent writing to file if same blkNum is received
	if (rcvBlk[1] == numLastGoodBlk) {
		goodBlk1st = false;
		// DO NOT return to catch corrupted bytes
//		return;
	}

	uint16_t checksum;		// used for both crc and xmodemchksum case
	if (Crcflg) {
		/* calculate and add CRC in network byte order */
		crc16ns(&checksum, &rcvBlk[DATA_POS]);
		uint8_t LSB = (uint8_t) (checksum >> 8);
		uint8_t MSB = (uint8_t) (checksum);
		// Assume big endian, then read from right to left
		// ie. MSB for 0xa3b4 is b4 (180 in decimal)
		if (!((MSB == rcvBlk[PAST_CHUNK]) && (LSB == rcvBlk[PAST_CHUNK + 1]))) {
			goodBlk1st = false;
			goodBlk = false;
		}
	} else {
		chksum8ns((uint8_t *)&checksum, &rcvBlk[DATA_POS]);
		if ((uint8_t) checksum != rcvBlk[PAST_CHUNK]) {
			goodBlk1st = false;
			goodBlk = false;
		}
	}

#ifdef _DEBUG
	cout << "Receiver: in getRestBlk(). rcvBlk contains ";
	cout << "[0]:" << ASCII((int)rcvBlk[0]) << " "; // SOH
	cout << "[1]:" << (int)rcvBlk[1] << " ";// blkNum
//	cout << "[2]:" << (int) rcvBlk[2] << " "; // 1's Complement
//	cout << "numLastGoodBlk:" << (int) numLastGoodBlk << " ";
//	cout << "CRCchecksum: " << CRCchecksum << " ";
//	cout << "msb: " << (int) MSB << " ";
//	cout << "lsb: " << (int) LSB << " ";
//	cout << "sum:" << SUM << " ";
//	cout << "chksum:" << chksum << endl;
	cout << endl;
#endif
}

//Write chunk (data) in a received block to diskddddddddd
void ReceiverX::writeChunk() {
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8() {
	// no need to space CAN chars coming from receiver
	const int canLen = 8; // move to defines.h
	char buffer[canLen];
	memset(buffer, CAN, canLen);
	PE_NOT(myWrite(mediumD, buffer, canLen), canLen);
}

