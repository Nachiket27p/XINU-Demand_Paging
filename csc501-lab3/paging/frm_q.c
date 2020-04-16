/* frm_q.c */
#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <frm_q.h>

int frm_q_head;
int frm_q_tail;
int SC_next_victim;

/*
 * initializes the queue which keeps track of the frames
 * for the SC replacement policy
 */
void init_frm_q()
{
    frm_q_head = -1;
    frm_q_tail = -1;
    SC_next_victim = -1;
}
