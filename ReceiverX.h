#ifndef RECEIVER_H
#define RECEIVER_H

#include "PeerX.h"

class ReceiverX : public PeerX
{
public:
	ReceiverX(int d, const char *fname, bool useCrc=true);

	void receiveFile();
	void getRestBlk();	// get the remaining 131 bytes of a block
	void writeChunk();
	void can8();		// send 8 CAN characters

	bool goodBlk; 		//indicates whether or not the last block received had problems.
	bool goodBlk1st;	// a particular block was received in good condition for the first time

	uint8_t NCGbyte;	// Either a 'C' or a NAK sent by receiver to initiate the file transfer

	/* A variable which counts the number of NAK characters in a
row sent because of problems like communication
problems. An initial NAK (or 'C') does not add to the count. The reception
of a particular block in good condition for the first time resets the count. */
//	unsigned errCnt;

private:
	// blkT rcvBlk;		// a received block
	uint8_t rcvBlk[BLK_SZ_CRC];		// a received block

	uint8_t numLastGoodBlk; // the number of the last good block

	//void checksum(uint8_t* sumP, blkT blkBuf);
};

#endif
