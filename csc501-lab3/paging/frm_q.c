/* frm_q.c */
#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <frm_q.h>

int frm_head;
int frm_tail;

/*
 * initializes the queue which keeps track of the frames
 * for the SC replacement policy
 */
void init_frm_q()
{
    frm_head = NFRAMES;
    frm_tail = NFRAMES + 1;
    frm_queue[frm_head].fr_next = frm_tail;
    frm_queue[frm_head].fr_prev = -1;
    frm_queue[frm_tail].fr_prev = frm_head;
    frm_queue[frm_tail].fr_next = -1;
}

