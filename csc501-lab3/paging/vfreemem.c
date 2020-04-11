/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;
	disable(ps);

	size = (unsigned)roundmb(size);

	// if the block requested to free is out invalid or
	// if the size of 0 than return SYSERR
	if(block < BVPN * NBPG || !size)
	{
		restore(ps);
		return SYSERR;
	}

	struct pentry *pptr = &proctab[currpid];
	
	struct mblock *vhCurr = pptr->vmemlist;
	struct mblock *vhNext = pptr->vmemlist->mnext;

	// Traverse through the list of virtual heap and locate the
	// next free slot
	while(vhNext != (struct mblock *)NULL && vhNext < block)
	{
		vhCurr = vhNext;
		vhNext = vhNext->mnext;
	}

	// if the next slot is the vmemlist head or if the next
	// is not null or the block + size is greater then
	// the next next heap than return syserr
	unsigned top = vhCurr->mlen + (unsigned)vhCurr;

	// make sure the block is being placed at the correct position in the list
	if((top > (unsigned)block && vhCurr != pptr->vmemlist) ||
		(vhNext != (struct mblock *)NULL && ((unsigned)block + size) > (unsigned)vhNext))
	{
		restore(ps);
		return SYSERR;
	}

	// update the virtual heap list to accomodate the freed
	// virtual heap block
	if(vhCurr != pptr->vmemlist && top == (unsigned)block)
	{
		vhCurr->mlen = size;
	}
	else
	{
		block->mlen = size;
		block->mnext = vhNext;
		vhCurr->mnext = block;
		vhCurr = block;
	}

	return(OK);
}
