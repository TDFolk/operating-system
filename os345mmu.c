// os345mmu.c - LC-3 Memory Management Unit
// **************************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345lc3.h"

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];

// statistics
int memAccess;						// memory accesses
int memHits;						// memory hits
int memPageFaults;					// memory faults
int nextPage;						// swap page size
int pageReads;						// page reads
int pageWrites;						// page writes
int cBigHand;
int cLittleHand;

int getFrame(int);
int getAvailableFrame(void);
void swapFrame(int entry1Index);
extern TCB tcb[];					// task control block
extern int curTask;					// current task #


int getFrame(int notme)
{
	int frame;
	
	frame = getAvailableFrame();
	if (frame >=0) return frame;

	// run clock
	int totalWrap = 20;

	while (totalWrap != 0) {
		int i;
		int rpte1;
		int upta;
		int upte1;

		if (cBigHand >= LC3_RPT_END) {
			cBigHand = LC3_RPT;
			totalWrap--;
		}

		rpte1 = memory[cBigHand];

		if (DEFINED(rpte1) && REFERENCED(rpte1)) {
			memory[cBigHand] = rpte1 = CLEAR_REF(rpte1);
		}
		else if (DEFINED(rpte1)) {
			upta = (FRAME(rpte1) << 6);

			for (i = cLittleHand % 64; i < 64;i += 2, cLittleHand = i % 64) {
				upte1 = memory[upta + (i)];
				if (PINNED(upte1) || FRAME(upte1) == notme) {
					cBigHand = cBigHand + 2;
					cLittleHand = 0;
					continue;
				}

				if (DEFINED(upte1) && REFERENCED(upte1)) {
					memory[cBigHand] = rpte1 = SET_PINNED(rpte1);
					memory[upta + (i)] = upte1 = CLEAR_REF(upte1);
				}
				else if (DEFINED(upte1)) {

					memory[cBigHand] = rpte1 = SET_DIRTY(rpte1);
					frame = FRAME(upte1);
					swapFrame(upta + i);
					cLittleHand += 2;

					break;
				}
			}

			cLittleHand = 0;
			if (!REFERENCED(rpte1) && !PINNED(rpte1) && FRAME(rpte1) != notme) {
				frame = FRAME(rpte1);

				swapFrame(cBigHand);
				cBigHand += 2;
				break;
			}
			else {
				memory[cBigHand] = rpte1 = CLEAR_PINNED(rpte1);
			}

		}

		cBigHand = cBigHand + 2;
		cLittleHand = 0;

	}

	return frame;
}

void swapFrame(int index)
{
    
	int access;
	int secondAccess;

    access = memory[index];
    secondAccess = memory[index + 1];

    
    if (DIRTY(access) && PAGED(secondAccess)) {
        accessPage(SWAPPAGE(secondAccess), FRAME(access), PAGE_OLD_WRITE);
    } else if (!PAGED(secondAccess)) {
        memory[index + 1] = secondAccess = SET_PAGED(nextPage);
        accessPage(nextPage, FRAME(access), PAGE_NEW_WRITE);
    }

    
    memory[index] = 0;
    

    return;
}
// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     / 	             / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

#define MMU_ENABLE	1

unsigned short int *getMemAdr(int va, int rwFlg)
{
	unsigned short int pa;
	int rpta, rpte1, rpte2;
	int upta, upte1, upte2;
	int rptFrame, uptFrame;
    memAccess += 2;

    rpta = tcb[curTask].RPT + RPTI(va);
	rpte1 = memory[rpta];
	rpte2 = memory[rpta+1];

	if (va < 0x3000) return &memory[va];
#if MMU_ENABLE
	if (DEFINED(rpte1))
	{
        memHits++;
	}
	else
	{
        memPageFaults++;
		rptFrame = getFrame(-1);
		rpte1 = SET_DEFINED(rptFrame);
		if (PAGED(rpte2))
		{
			accessPage(SWAPPAGE(rpte2), rptFrame, PAGE_READ);
		}
		else
		{
            memset(&memory[(rptFrame<<6)], 0, 128);
		}
	}


	memory[rpta] = rpte1 = SET_REF(rpte1);
	memory[rpta+1] = rpte2;

	upta = (FRAME(rpte1)<<6) + UPTI(va);
	upte1 = memory[upta];
	upte2 = memory[upta+1];

	if (DEFINED(upte1))
	{
        memHits++;
	}
	else
	{
        memPageFaults++;
		uptFrame = getFrame(FRAME(memory[rpta]));
        memory[rpta] = rpte1 = SET_REF(SET_DIRTY(rpte1));
        upte1 = SET_DEFINED(uptFrame);

        if (PAGED(upte2))
		{
			accessPage(SWAPPAGE(upte2), uptFrame, PAGE_READ);
		}
		else
		{
            memset(&memory[(uptFrame<<6)], 0xf025, 128);
		}
	}

    if (rwFlg) {
        upte1 = SET_DIRTY(upte1);
    }

    memory[upta] = SET_REF(upte1);
	memory[upta+1] = upte2;

	return &memory[(FRAME(upte1)<<6) + FRAMEOFFSET(va)];
#else
	return &memory[va];
#endif
} // end getMemAdr


// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
void setFrameTableBits(int flg, int sf, int ef)
{	int i, data;
	int adr = LC3_FBT-1;             // index to frame bit table
	int fmask = 0x0001;              // bit mask

	// 1024 frames in LC-3 memory
	for (i=0; i<LC3_FRAMES; i++)
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;
			adr++;
			data = (flg)?MEMWORD(adr):0;
		}
		else fmask = fmask >> 1;
		// allocate frame if in range
		if ( (i >= sf) && (i < ef)) data = data | fmask;
		MEMWORD(adr) = data;
	}
	return;
} // end setFrameTableBits


// **************************************************************************
// get frame from frame bit table (else return -1)
int getAvailableFrame()
{
	int i, data;
	int adr = LC3_FBT - 1;				// index to frame bit table
	int fmask = 0x0001;					// bit mask

	for (i=0; i<LC3_FRAMES; i++)		// look thru all frames
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;				// move to next word
			adr++;
			data = MEMWORD(adr);
		}
		else fmask = fmask >> 1;		// next frame
		// deallocate frame and return frame #
		if (data & fmask)
		{  MEMWORD(adr) = data & ~fmask;
			return i;
		}
	}
	return -1;
} // end getAvailableFrame



// **************************************************************************
// read/write to swap space
int accessPage(int pnum, int frame, int rwnFlg)
{
   static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];

   if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE))
   {
      printf("\nVirtual Memory Space Exceeded!  (%d) - requested nextPage %d, pnum %d", LC3_MAX_PAGE, nextPage, pnum);
      exit(-4);
   }
   switch(rwnFlg)
   {
      case PAGE_INIT:                    		// init paging
         nextPage = 0;
         return 0;

      case PAGE_GET_ADR:                    	// return page address
         return (int)(&swapMemory[pnum<<6]);

      case PAGE_NEW_WRITE:                   // new write (Drops thru to write old)
         pnum = nextPage++;

      case PAGE_OLD_WRITE:                   // write
         //printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
         memcpy(&swapMemory[pnum<<6], &memory[frame<<6], 1<<7);
         pageWrites++;
         return pnum;

      case PAGE_READ:                    // read
         //printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
      	 memcpy(&memory[frame<<6], &swapMemory[pnum<<6], 1<<7);
         pageReads++;
         return pnum;

      case PAGE_FREE:                   // free page
         memset(&memory[(frame<<6)], 0xf025, 128);
         break;
   }
   return pnum;
} // end accessPage
