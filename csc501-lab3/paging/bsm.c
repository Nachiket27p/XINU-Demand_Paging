/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    // disable interrupts
    STATWORD ps;
    disable(ps);
    
	// get prointer to backing store map entry store entry
    bs_map_t *bsptr;
    
	int i;
    for(i = 0; i < NBS; i++)
    {
		bsptr = &bsm_tab[i];

    	bsptr->bs_status = BSM_UNMAPPED;
    	// bsptr->bs_pid = NOVAL;
		bsptr->bs_pid_track = 0;
    	bsptr->bs_vpno = BVPN;
    	bsptr->bs_npages = 0;
    	// bsptr->bs_sem = 0;
    	bsptr->bs_priv = 0;
    }

	// restore interrupts
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    // disable interrupts
    STATWORD ps;
    disable(ps);
    SYSCALL rtnVal = SYSERR;
	
	// get prointer to backing store map entry store entry
    bs_map_t *bsptr;

    int i = 0;
    while(i < NBS)
    {
		bsptr = &bsm_tab[i];
        if(bsptr->bs_status == BSM_UNMAPPED)
        {
            // set the value to the first available backing store
            *avail = i;
            // set return value and break out of while loop
            rtnVal = OK;
            break;
        }
        i++;
    }

    // restore interrupts
    restore(ps);
    return rtnVal;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    // disable interrupts
    STATWORD ps;
    disable(ps);
    
    // check the bs id procided is valid
    if(bad_bs_id(i))
    {
        restore(ps);
        return SYSERR;
    }

    // get prointer to backing store map entry store entry
    bs_map_t *bsptr = &bsm_tab[i];

    // update the backing backing store map entry to indicat its been freed
    bsptr->bs_status = BSM_UNMAPPED;
	// bsptr->bs_pid = NOVAL;
	bsptr->bs_pid_track = 0;
    bsptr->bs_vpno = BVPN;
	bsptr->bs_npages = 0;
	// bsptr->bs_sem = 0;
	bsptr->bs_priv = 0;

    // restore interrupts
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    // disable interrupts
    STATWORD ps;
    disable(ps);
    
    // u_long vpno = vaddr/NBPG;
    bs_map_t *bsptr;
    SYSCALL rtnVal = SYSERR;
    
    int i = 0;
    while(i < NBS)
    {
        // get prointer to backing store map entry store entry
        bsptr = &bsm_tab[i];
        // check if the bitmas has the correct bit set for this pid
        
        // if(bsptr->bs_pid == pid)
		if((bsptr->bs_pid_track >> pid) & (u_llong)1)
        {
			*store = i;
			// provide the page
			*pageth = (vaddr/NBPG) - bsptr->bs_vpno;
			// set return value and break out of while loop
			rtnVal = OK;
			break;

        }
        i++;
    }

    // restore interrupts
    restore(ps);
    return rtnVal;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	// disable interrupts
    STATWORD ps;
    disable(ps);

    // check that the 
    if(bad_bs_npage(npages) || bad_bs_id(source))
    {
        restore(ps);
        return SYSERR;
    }

	// update the proc table at entry 'pid' that it using this backing store
    proctab[pid].store |= ((u_int)1 << source);
    proctab[pid].vhpno = vpno;

    // get prointer to backing store map entry store entry
    bs_map_t *bsptr =  &bsm_tab[source];
    
    // update the backing store map entry to indicate its been mapped to
    // process for a specific number of pages
    bsptr->bs_status = BSM_MAPPED;
    // bsptr->bs_pid = pid;
	bsptr->bs_pid_track |= ((u_llong)1 << pid);
    bsptr->bs_vpno = vpno;
    bsptr->bs_npages = npages;
    // bsptr->bs_sem = 1;
    bsptr->bs_priv = BS_SHARED;
    
    // restore interrupts
    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	// disable interrupts
    STATWORD ps;
    disable(ps);

    // check is pid is valid
    if(isbadpid(pid))
    {
        restore(ps);
        return SYSERR;
    }

    int i, store, pageth;

	fr_map_t *frptr;
	// iterate through all the frames for this backing store and
	// write back to the backing store
    for(i = 0; i < NFRAMES; i++)
    {
		frptr = &frm_tab[i];
        // also not sure if i need to check "fr_dirty" or "pt_dirty" to write out to backing store
        if(frptr->fr_pid == pid && frptr->fr_type == FR_PAGE && frptr->fr_dirty)
        {
            bsm_lookup(pid, vpno * NBPG, &store, &pageth);
            write_bs( (i + NFRAMES) * NBPG, store, pageth);
        }
    }

    bs_map_t *bsptr =  &bsm_tab[store];

    bsptr->bs_status = BSM_UNMAPPED;
    // bsptr->bs_pid = NOVAL;
	bsptr->bs_pid_track &= (!((u_llong)1 << pid));
    bsptr->bs_vpno = BVPN;
	bsptr->bs_npages = 0;
	// bsptr->bs_sem = 0;
	bsptr->bs_priv = 0;

    // remove the tracker from this processes process table entry
    proctab[pid].store &= (!((u_int)1 << store));
        
    restore(ps);
    return OK;
}


