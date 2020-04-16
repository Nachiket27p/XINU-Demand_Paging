/* frm_q.h */

// struct to keep track of frames for replacement policy
typedef struct {
    int fr_next;
    int fr_prev;
} fr_q_node;

// extern variables
extern fr_q_node frm_queue[];
extern int frm_head;
extern int frm_tail;

void init_frm_q();


// inlined functions
#define first_frm_id_q(fqhead) frm_queue[(fqhead)].fr_next
#define last_frm_id_q(fqtail) frm_queue[(fqtail)].fr_prev
#define next_frm_id_q(curr) frm_queue[(curr)].fr_next
#define isEmpty_frm_q frm_queue[frm_head].fr_next == frm_tail

/*
 * Inserts the fn into the tail end of the queue
 * fn - the frame which needs to be inserted into the queue
 */
#define insert_frm_q(fn)\
    frm_queue[fn].fr_prev = frm_queue[frm_tail].fr_prev;\
    frm_queue[frm_queue[frm_tail].fr_prev].fr_next = fn;\
    frm_queue[fn].fr_next = frm_tail;\
    frm_queue[frm_tail].fr_prev = fn;

/*
 * Removes teh frame (frm) from the queue
 * frn - the frame id which needs to be removed from the queue
 * fn - the variable in which the removed frame id will be stored
 */
#define remove_frm_q(frn)\
    fr_q_node *fptr = &frm_queue[(frn)];\
    frm_queue[fptr->fr_prev].fr_next = fptr->fr_next;\
    frm_queue[fptr->fr_next].fr_prev = fptr->fr_prev;


