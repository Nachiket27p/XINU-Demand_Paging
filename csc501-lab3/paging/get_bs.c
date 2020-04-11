#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages)
{
	STATWORD ps;
    disable(ps);

    if(bad_bs_npage(npages) || bad_bs_id(bs_id))
    {
        restore(ps);
        return SYSERR;
    }

	bs_map_t *bsptr = &bsm_tab[bs_id];

	// update this processes pid that it has this store mapped
	proctab[currpid].store |= ((u_int)1 << bs_id);
    
	// if the backing store in unmapped than than map it
    // and return the the number of requested pages
    if(bsptr->bs_status == BSM_UNMAPPED)
    {
        bsptr->bs_status = BSM_MAPPED;
        // bsptr->bs_pid = currpid;
		bsptr->bs_pid_track |= ((u_llong)1 << currpid);

        restore(ps);
        return npages;
    }
    else
    {
        // if the store is mapped but is private than return SYSERR
        if(bsptr->bs_priv == BS_PRIV)
        {
            restore(ps);
            return SYSERR;
        }

        // update backing store map to incidate this process has this store mapped
        // bsptr->bs_pid = currpid;
		bsptr->bs_pid_track |= ((u_llong)1 << currpid);

        restore(ps);
        return bsptr->bs_npages;
    }
}


