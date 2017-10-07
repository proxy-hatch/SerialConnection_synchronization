//============================================================================
//
//% Student Name 1: Shawn (Yu Xuan) Wang
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
// Version     : September 3rd, 2017
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

ReceiverX::ReceiverX(int d, const char *fname, bool useCrc) :
		PeerX(d, fname, useCrc), goodBlk(false), goodBlk1st(false), numLastGoodBlk(
				0) {
	NCGbyte = useCrc ? 'C' : NAK;
}

void ReceiverX::receiveFile() {
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	// should we test for error and set result to "OpenError" if necessary?
	transferringFileD = PE2(creat(fileName, mode), fileName);

	/* ques:
	 * sender and receiver are both sending can8 and die
	 * receiver has its own errCnt variable
	 */
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
	State state = SOHEOT;
	bool done = false;
	errCnt = 0; // variable not initialized in constructor

	//int EOTCount = 0;
	while (!done) { //(EOTCount < 2)

		/*
		 if (EOTCount == 1) {
		 if (rcvBlk[0] == EOT) {
		 sendByte(ACK); // ACK the second EOT
		 EOTCount++;
		 break;
		 } else
		 EOTCount = 0;
		 } */

		try {
			switch (state) {
			case SOHEOT: {
#ifdef _DEBUG
				cout << "Receiver: ";
				cout << "@SOHEOT" << " received " << (int) rcvBlk[0] << " <- "
						<< (int) rcvBlk[1] << endl;
#endif
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);

				if (rcvBlk[0] == SOH) {
					getRestBlk();
					state = TRAN;
				} else if (rcvBlk[0] == EOT) {
					//EOTCount++;
					sendByte(NAK);
					state = EOTEOT;
				} else if (rcvBlk[0] == CANC) {
					state = CANC;
				}
				break;
			}
			case TRAN: {
#ifdef _DEBUG
				//cout << "Receiver: ";
				//cout << "@TRAN" << endl;
#endif
				if (errCnt <= errB) {

					if (goodBlk) {
#ifdef _DEBUG
						//cout << "Receiver: ";
						//cout << "sends ACK" << endl;
						cout << endl;
#endif
						sendByte(ACK);
						if (goodBlk1st) {

							/* Corner case. */
							if (rcvBlk[1] >= 255) {
								numLastGoodBlk = rcvBlk[1] - 255;
							} else {
								numLastGoodBlk = rcvBlk[1];
							}
							writeChunk();
							errCnt = 0;
						}
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
				} else {
					can8();

					if (errCnt == errB + 2)
						result = "RcvCancelled";
					else
						result = "ScdCancelled";
					done = true;
				}
				break;
			}
			case EOTEOT: {
#ifdef _DEBUG
				cout << "Receiver: ";
				cout << "@SOHEOT" << " received " << (int) rcvBlk[0] << endl;
#endif
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
				if (rcvBlk[0] == EOT) {
					sendByte(ACK);
					result = "Done";
					done = true;
				}
				break;
			}
			case CANC: {
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
				if (rcvBlk[0] == CAN) {
					result = "Done";
					done = true;
				}
				break;
			}
			}

		} catch (...) {
			//cerr << "Sender received totally unexpected char #" << rcvBlk[0]
			//		<< ": " << (unsigned) rcvBlk[0] << endl;
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
	/*
	 * Ques: 1. Do we remove padding? 2. should we test for error and set result to "OpenError" if necessary?
	 * what is "an appropriate time?
	 */

	// ********* this function must be improved ***********
	PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0),
			REST_BLK_SZ_CRC);
	goodBlk1st = goodBlk = true; // True by default. Only set to false when conditions are met

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
	if (rcvBlk[1] != numLastGoodBlk && rcvBlk[1] != numLastGoodBlk + 1) {
		//can8();

		cout << endl << "Receiver: sends abort. " << endl;
		errCnt = errB + 2;
		return;
	}

	// Prevent writing to file if same blkNum is received
	if (rcvBlk[1] == numLastGoodBlk) {
		goodBlk1st = false;
		//goodBlk = true;
		return;
	}

	// Add up all the bytes in the received chunk together
	uint8_t chksum = rcvBlk[PAST_CHUNK];
	uint8_t LSB, MSB, SUM;
	uint16_t CRCchecksum;

	// If LSB(SUM)=checksum, allow appending bytes to file
	if (NCGbyte == 'C') {
		/* calculate and add CRC in network byte order */
		crc16ns(&CRCchecksum, &rcvBlk[DATA_POS]);
		uint8_t LSB = (uint8_t) (CRCchecksum >> 8);
		uint8_t MSB = (uint8_t) (CRCchecksum);
		// Assume big endian, then read from right to left
		// ie. MSB for 0xa3b4 is b4 (180 in decimal)

		if (!((MSB == rcvBlk[PAST_CHUNK]) && (LSB == rcvBlk[PAST_CHUNK + 1]))) {
			//goodBlk1st = true;
			//}
			//else{
			goodBlk1st = false;
		}
	} else if (NCGbyte == NAK) {
		// compares the least significant byte of the Sum with the checksum
		for (int ii = DATA_POS + 1; ii < DATA_POS + CHUNK_SZ; ii++)
			SUM += rcvBlk[ii];

		if (SUM != chksum) {
			//	goodBlk1st = true;
			//} else {
			goodBlk1st = false;
		}
	}

#ifdef _DEBUG
	cout << "Receiver: in getRestBlk(). rcvBlk contains ";
	cout << "[0]:" << (int) rcvBlk[0] << " ";
	cout << "[1]:" << (int) rcvBlk[1] << " ";
//	cout << "[2]:" << (int) rcvBlk[2] << " ";
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

