/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <frm_q.h>

LOCAL SYSCALL sc_replacement_policy();
LOCAL SYSCALL lfu_replacement_policy();
SYSCALL free_frm(int i);

int frm_head;
int frm_tail;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
    disable(ps);

    fr_map_t *frptr;
    int i;
    for(i = 0; i < NFRAMES; i++)
    {
        frptr = &frm_tab[i];

        frptr->fr_status = FRM_UNMAPPED;
        frptr->fr_pid = NOVAL;
        frptr->fr_vpno = 0;
        frptr->fr_refcnt = 0;
        frptr->fr_type = FR_PAGE;
        frptr->fr_dirty = 0;
    }
    
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  	STATWORD ps;
  	disable(ps);
  	int i = 0;

  	// check if a frame is available if so return it
  	while(i < NFRAMES){
  		if(frm_tab[i].fr_status == FRM_UNMAPPED){
  			*avail = i;
  			restore(ps);
  			return OK;
  		}
  		i++;
  	}

	int frameNumb;
    // based on the page replacement policy get a frame
    if(page_replace_policy == SC)
    {
        frameNumb = sc_replacement_policy();
    }
    else if(page_replace_policy == LFU)
    {
        frameNumb = lfu_replacement_policy();
    }

	// free the frame and return it to be used
  	free_frm(frameNumb);
    *avail = frameNumb;
	
  	restore(ps);
  	return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
     STATWORD ps;
    disable(ps);

    if(frm_tab[i].fr_status == FRM_UNMAPPED || bad_frame_numb(i) )
    {
        restore(ps);
        return SYSERR;
    }

	u_long pdbr;	
	u_int pdOffset, ptOffset;
	

	pd_t *pdptr; 
	pt_t *ptptr;
	int bStore, pn, fppid;

	fr_map_t *frptr = &frm_tab[i];
    int frType = frptr->fr_type;

	virt_addr_t *virtAddr;

	if(frType == FR_PAGE)
	{
		fppid = frptr->fr_pid;
		pdbr = proctab[fppid].pdbr;

		// Typecast the vpno to the virt_addr_t struct to easily
        // extract the pd offset and pt offset
		virtAddr = (virt_addr_t *)(&(frptr->fr_vpno));

        pdOffset = virtAddr->pd_offset;
        ptOffset = virtAddr->pt_offset;
		
		pdptr = pdbr + (pdOffset * sizeof(pd_t));
		ptptr = (pdptr->pd_base * NBPG) + (ptOffset * sizeof(pt_t));

        // // locate the correct backing store for this frame so if it needs to be written back
        // // to the backing store its written to the correct backin store because 
        // // a process can have multiple multiple backing stores
        // for(bStore = 0; bStore < NBS; bStore++)
        // {
        //     if(((proctab[frptr->fr_pid].store >> bStore) & (u_int)1) &&
        //         (frptr->fr_vpno >= bsm_tab[bStore].bs_vpno &&
        //             frptr->fr_vpno < (bsm_tab[bStore].bs_vpno + BACKING_STORE_UNIT_SIZE) ))
        //     {
        //         break;
        //     }
        // }
		// // bStore = proctab[frptr->fr_pid].store;
        // pn = frptr->fr_vpno - proctab[fppid].vhpno;

        // use bsm_lookup to get the correct backins store and the pageth
        bsm_lookup(currpid, virtAddr, &bStore, &pn);
		
		if(bad_frame_numb(i))
		{
			restore(ps);
			return SYSERR;
		}
// ???????????????????????
// not sure i need to check fr_dirty or pt_dirty
		// if the frame is dirty write back to backing store
		if(ptptr->pt_dirty || frptr->fr_dirty)
		{
            ptptr->pt_dirty = 0;
            frptr->fr_dirty = 0;
			write_bs((i + FRAME0) * NBPG, bStore, pn);
		}

		// update the page table to indicate this frame is no longer mapped
		ptptr->pt_pres = 0;

		// get a pointer to the frame
		frptr = &frm_tab[pdptr->pd_base - FRAME0];

		frptr->fr_refcnt--;
		
		if(frptr->fr_refcnt == 0)
		{
			if(page_replace_policy == LFU)
            {
				// TODO?
			}
			else if(page_replace_policy == SC)
            {
				// TODO?
			}
			frptr->fr_pid = NOVAL;
			frptr->fr_status = FRM_UNMAPPED;
			frptr->fr_type = FR_PAGE;
			frptr->fr_vpno = BVPN;
			pdptr->pd_pres = 0;
		}
	}
	else if(frType == FR_TBL && i < FR_TBL)
	{
		restore(ps);
		return SYSERR;
	}
	else if(frType == FR_DIR && i < FR_DIR)
	{
		restore(ps);
		return SYSERR;
	}

 	restore(ps);
	return OK;
}

LOCAL SYSCALL sc_replacement_policy()
{
	// disable interrupts
    STATWORD ps;
    disable(ps);

    // used to store frame frame number which will be replaces
    int frameNumb = -1;

    u_long frVpno;
    u_int ptOffset, pdOffset;

    virt_addr_t *virtAddrT;

    pd_t *pdptr;
	pt_t *ptptr;

    // set the current frame number to dummy/null node
    int currFN = frm_head;

    // keep looking for a frame from the frame queue
    while(frameNumb == -1)
    {
        // get the actual first node in the frame queue
        currFN = next_frm_id_q(currFN);

        // get the frame virtual page number
        frVpno = frm_tab[currFN].fr_vpno;

        // cast the address of the frame number to the struct virt_addr_t
        // so that the page table offset and page directory offset can be
        // obtained easily
        virtAddrT = (virt_addr_t*)(&frVpno);
        ptOffset = virtAddrT->pt_offset;
        pdOffset = virtAddrT->pd_offset;
        
        // get pointer to page directory
        pdptr = (pd_t*)(proctab[currpid].pdbr + (pdOffset * sizeof(pd_t)));

        // use the page directory to get the pointer to page table
        ptptr = (pt_t*)((pdptr->pd_base * NBPG) + (ptOffset * sizeof(pt_t)));

        // if the reference bit is not set than remove the frame from
        // the frame queue
        if(ptptr->pt_acc == 0)
        {
            // set frame number to break out of the loop and return
            frameNumb = currFN;
            // if debugging is turned on than print the replaced frame
            if(debugging)
            {
                kprintf("Replaced Frame: 0x%08x\n", frameNumb);
            }
        }
        else
        {
            // pt_acc is 1 that means the page has been references
            // clear the reference so that next time it can be replaced
            ptptr->pt_acc = 0;
        }
    }

    restore(ps);
    return frameNumb;
}

LOCAL SYSCALL lfu_replacement_policy()
{
    STATWORD ps;
    disable(ps);
    
    u_int minAcc = MAXUINT;
    int currFN = -1;
    int i;

    // loop through all the frames and determine which one has the lowest
    // accesses, and return the frame number
    for(i = 0; i < NFRAMES; i++)
    {
        fr_map_t *frptr = &frm_tab[i];
        if(frptr->fr_type == FR_PAGE)
        {
            if(frptr->fr_refcnt < minAcc)
            {
                minAcc = frptr->fr_refcnt;
                currFN = i;
            }
            else if(frptr->fr_refcnt == minAcc && frptr->fr_vpno > frm_tab[currFN].fr_vpno)
            {
                currFN = i;
            }
        }
    }

    // if debugging is turned on than print the replaced frame
    if(debugging)
    {
        kprintf("Replaced Frame: 0x%08x\n", currFN);
    }

    restore(ps);
    return currFN;
}

