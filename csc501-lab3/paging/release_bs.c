#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id)
{
	/* release the backing store with ID bs_id */
    STATWORD ps;
    disable(ps);
 
    bs_map_t *bsptr = &bsm_tab[bs_id];

    // if(bad_bs_id(bs_id) || bsptr->bs_status == BSM_UNMAPPED ||
	// 	(bsptr->bs_priv == BS_PRIV && bsptr->bs_pid != currpid))
	if(bad_bs_id(bs_id) || bsptr->bs_status == BSM_UNMAPPED ||
		(bsptr->bs_priv == BS_PRIV && ((bsptr->bs_pid_track >> currpid) & (u_llong)1) == 0))
    {
        restore(ps);
        return SYSERR;
    }


	if(bsptr->bs_priv == BS_PRIV)
    {
        free_bsm(bs_id);
    }
	else
    {
        // bsptr->bs_pid = -1;
		bsptr->bs_pid_track &= (!((u_llong)1 << currpid));

        // update the proctable to indicate the process not longer has this backing store
        proctab[currpid].store &= (!((u_int)1 << bs_id));
        proctab[currpid].vhpno = 0;
        
        // now check if the number of processes mapped to this bakcing store is 0
        // if so free the backing store
		if(bsptr->bs_pid_track == 0)
		{
        	free_bsm(bs_id);
		}
    }

	return(OK);
}

