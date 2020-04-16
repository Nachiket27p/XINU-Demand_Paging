/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);

	int pid, bs;
	
	//check if a particular backing store can be acquired
	if(bad_bs_npage(hsize) || get_bsm(&bs) == SYSERR)
	{
		restore(ps);
		return SYSERR;
	}

	pid = create(procaddr, ssize, priority, name, nargs, args);
	
	// if the process could not be created than return syserr
	if(pid == SYSERR)
	{
		restore(ps);
		return SYSERR;
	}

	// map the backing store located at the 'bs' entry in the
	// backing store table
	bsm_map(pid, BVPN, bs, hsize);

	// get pointer to the process table entry
	struct pentry *pptr = &proctab[pid];

	// pptr->store = bs;
	// pptr->vhpno = BVPN;
	pptr->vhpnpages = hsize;
	pptr->vmemlist->mnext = BVPN * NBPG;
	
	// get pointer to the memory block for this process
	struct mblock *mblk;
	
	mblk = BACKING_STORE_BASE + (bs * BACKING_STORE_UNIT_SIZE);
	mblk->mlen = hsize * NBPG;
	mblk->mnext = NULL;

	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
