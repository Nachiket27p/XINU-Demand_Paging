/*  pd_alloc.c   */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
 * Allocated the page directory for a given process by locating a 
 * free frame or replacing one using SC or LFU policy.
 *
 * pid - process id of the process for which the page directory is allocated
 */
void pd_alloc(int pid)
{
	STATWORD ps;
    disable(ps);

    int i, frameNumb;
    
    // find a unused frame or replace another frame
    get_frm(&frameNumb);

    // update the process table to hold the new process directory base register
    proctab[pid].pdbr = (FRAME0 + frameNumb) * NBPG;

    // Get the pointer to the frame and update it to indicate that it
    // is mapped and set the pid to the process which is mapped it it
    // Set flag to indicate the frame is being used as a directory
    fr_map_t *frptr = &frm_tab[frameNumb];
    frptr->fr_status = FRM_MAPPED;
    frptr->fr_pid = pid;
    frptr->fr_type = FR_DIR;

    pd_t *pdptr;
    u_int *pdAsInt;

    // get pointer to the first page directory
    pdptr = (pd_t *)proctab[pid].pdbr;

    // set the first 4 frames for the page directory
    for(i = 0; i < 4; i++)
    {
        // set all fields to 0 by type casting the page table entry
        // to an u_int and set to zero
        pdAsInt = (u_int *)pdptr;
        *pdAsInt = 0;
        
        // Type cast to pd_t struct to set specific elements
        pdptr = (pd_t *)pdAsInt;

        pdptr->pd_pres = 1;
        pdptr->pd_write = 1;
        pdptr->pd_base = FRAME0 + i;

        // increment pointer
        pdptr++;
    }

    // set the rest of the frames
    for(i = 4; i < NFRAMES; i++)
    {
        // To set all elements of the struct to 0, typecast the pointer to the
        // struct to an (u_int *) and set it to 0 because the struct is exactly 32 bits.
        pdAsInt = (u_int *)pdptr;
        *pdAsInt = 0;

        // increment pointer
        pdptr++;
    }

    restore(ps);
}