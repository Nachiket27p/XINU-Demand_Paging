/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
	disable(ps);

	// get pointer to process table entry
	struct pentry *pptr = &proctab[currpid];

	if(pptr->vmemlist->mnext == (struct mblock*)NULL || nbytes <= 0)
	{
		restore(ps);
		return SYSERR;
	}

	// round or truncate address up to size of mblock
	nbytes = (u_int) roundmb(nbytes);

	struct mblock *tmp;

	struct mblock *vhCurr = pptr->vmemlist;
	struct mblock *vhNext = pptr->vmemlist->mnext;

	// Traverse through the list of virtual heap to find a free slot
	while(vhNext != (struct mblock *)NULL)
	{
		// Is the next block has the the required nbytes
		// return the pointer
		if(vhNext->mlen == nbytes)
		{
			vhCurr->mnext = vhNext->mnext;
			restore(ps);
			return (WORD *)vhNext;
		}
		else if(vhNext->mlen > nbytes)
		{
			tmp = (struct mblock *)((unsigned)vhNext + nbytes);
			vhCurr->mnext = tmp;
			tmp->mnext = vhNext->mnext;
			tmp->mlen = vhNext->mlen - nbytes;
			restore(ps);
			return (WORD *)vhNext;
		}

		vhCurr = vhNext;
		vhNext = vhNext->mnext;
	}

	// if no place could be found in the virtual heap return SYSERR
	restore(ps);
	return (WORD *)SYSERR;
}


