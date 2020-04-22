/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
    STATWORD ps;
    disable(ps);
    
    SYSCALL rtn = OK;
    
    if( bsm_tab[source].bs_status == BSM_UNMAPPED ||
        bsm_tab[source].bs_priv ||
        bsm_map(currpid, virtpage, source, npages) == SYSERR )
    {
        rtn = SYSERR;
    }

    restore(ps);
    return rtn;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    disable(ps);
    bsm_tab[currpid].bs_pid_track &= (!((u_llong)1 << currpid));
    SYSCALL rtn = bsm_unmap(currpid, virtpage, 0);

    restore(ps);
    return rtn;
}
