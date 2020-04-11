/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <frm_q.h>

LOCAL SYSCALL get_new_page_table();
void initialize_page_table(int i);

#define NOT_FOUND -1


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
    STATWORD ps;
	disable(ps);

	
	// read the address in control reg 2
	// obtain the page table offset and page directory offset
	u_long va = read_cr2();

	virt_addr_t *vAddr = (virt_addr_t*)&va;
	u_int ptOffset = vAddr->pt_offset;
	u_int pdOffset = vAddr->pd_offset;

	// get page directory base register value for the current process
	u_long pdbr = proctab[currpid].pdbr;

	// get pointer to the page directory for the current process
	pd_t *pdptr = pdbr + (pdOffset * sizeof(pd_t));

	int newPT;
	// if the page directory is not present than get a new
	// page table and initialize it as a page directory
	if(pdptr->pd_pres == 0)
	{	
		// get a new page table to initialize for this fault
		newPT = get_new_page_table();

		pdptr->pd_pres = 1;
		pdptr->pd_write = 1;
		pdptr->pd_user = 0;
		pdptr->pd_pwt = 0;
		pdptr->pd_pcd = 0;
		pdptr->pd_acc = 0;
		pdptr->pd_mbz = 0;
		pdptr->pd_fmb = 0;
		pdptr->pd_global = 0;
		pdptr->pd_avail = 0;
		pdptr->pd_base = FRAME0 + newPT;

		frm_tab[newPT].fr_type = FR_TBL;
		frm_tab[newPT].fr_status = FRM_MAPPED;
		frm_tab[newPT].fr_pid = currpid;
	}

	// get pointer to the page table
	pt_t *ptptr = (pt_t*)((pdptr->pd_base * NBPG) + (ptOffset * sizeof(pt_t)));
	
	int newFr, pgOffset, bs_number;
	// If a page table is not present than get a new frame an
	// initialize the frame
	if(ptptr->pt_pres == 0)
	{
		get_frm(&newFr);

		ptptr->pt_pres = 1;
		ptptr->pt_write = 1;
		ptptr->pt_base = FRAME0 + newFr;

		frm_tab[pdptr->pd_base - FRAME0].fr_refcnt++;
		frm_tab[newFr].fr_status = FRM_MAPPED;
		frm_tab[newFr].fr_type = FR_PAGE;
		frm_tab[newFr].fr_pid = currpid;
		frm_tab[newFr].fr_vpno = va/NBPG;

		bsm_lookup(currpid, va, &bs_number, &pgOffset);

		read_bs((char*)((FRAME0 + newFr) * NBPG), bs_number, pgOffset);

		if(page_replace_policy == SC)
		{
			insert_frm_q(newFr);
		}
		else if(page_replace_policy == LFU)
		{
			// Nothing to do?
		}
	}

	write_cr3(pdbr);
	
	restore(ps);
  	return OK;
}

/*
 * Get a new page table to use and intitialize all the fields of 
 * the pt_t structure to zeros, even though they should all
 * be 0 but to be extra careful
 */
LOCAL SYSCALL get_new_page_table()
{
	int fn;
    
    // get a frame to allocate this page table inside
	get_frm(&fn);

    // access the page table struct pt_t as an u_int pointer because
    // they are both 32 bits long, so you can set all the bit fields
    // to zero with a single statement
	u_int *pageTab = (u_int*)((FRAME0 + fn) * NBPG);

	int i;
	for(i = 0; i < NFRAMES; i++)
    {
        *pageTab = 0;
		pageTab++;
	}
	//initialize_page_table(i);
	return fn;
}

