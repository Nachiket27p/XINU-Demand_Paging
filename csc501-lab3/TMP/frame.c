/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <frm_q.h>

LOCAL SYSCALL sc_replacement_policy();
LOCAL SYSCALL lfu_replacement_policy();
SYSCALL free_frm(int i);

int frm_q_head;
int frm_q_tail;

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
  	int i;

  	// check if a frame is available if so return it
    for(i = 0; i < NFRAMES; i++)
    {
  		if(frm_tab[i].fr_status == FRM_UNMAPPED)
        {
  			*avail = i;
  			restore(ps);
  			return OK;
  		}
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

        // use bsm_lookup to get the correct backins store and the pageth
        bsm_lookup(currpid, virtAddr, &bStore, &pn);
		
		if(bad_frame_numb(i))
		{
			restore(ps);
			return SYSERR;
		}

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

    // get the head of the frame queue because SC policy is implemented
    // based on FIFOreplacement with a second chance
    int currFN = frm_q_head;

    // get the frame behind the head which is the tail pointer
    // !!( Refer to 'frm_q.h - frmq_remove()' to see why previous is required.)!!
    int prevFN = frm_q_tail;

    // keep looking for a frame from the frame queue
    while(frameNumb == -1)
    {
        kprintf("OK");
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
                kprintf("Replaced Frame: %d\n", frameNumb);
            }
        }
        else
        {
            // pt_acc is 1 that means the page has been references
            // clear the reference so that next time it can be replaced
            ptptr->pt_acc = 0;

            // update the previous and current such that when the loop
            // breaks the prev points to frame before the one which is to
            // be removed.
            // !!( Refer to 'frm_q.h - frmq_remove()' to see why previous is required.)!!
            prevFN = currFN;
            currFN = frmq_next_id(currFN);
        }

    }

    frmq_remove(prevFN);

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
        kprintf("Replaced Frame: %d\n", currFN);
    }

    restore(ps);
    return currFN;
}

