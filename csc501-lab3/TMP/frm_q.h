/* frm_q.h - A circular queue which is used for Second Chance replacement policy*/

// struct to keep track of frames for replacement policy
typedef struct {
    int frq_next;
} fr_q_node;

// extern variables
extern fr_q_node frm_queue[];
extern int frm_q_head;
extern int frm_q_tail;

void init_frm_q();

/* ------------------------------------------------------- */
/*                  Inlined functions                      */
/* ------------------------------------------------------- */

#define frmq_first_id(fqhead) frm_queue[(fqhead)].frq_next
#define frmq_next_id(curr) frm_queue[(curr)].frq_next
#define frmq_isEmpty() (frm_q_head == -1 && frm_q_tail == -1)

/*
 * Inserts the fn into the tail end of the queue
 * fn - the frame which needs to be inserted into the queue
 */
#define frmq_insert(fn)\
    if(frm_q_head == -1)\
    {\
        frm_q_head = (fn);\
        frm_q_tail = (fn);\
        frm_queue[(fn)].frq_next = (fn);\
    }\
    else\
    {\
        frm_queue[frm_q_tail].frq_next = (fn);\
        frm_queue[(fn)].frq_next = frm_q_head;\
        frm_q_tail = (fn);\
    }

/*
 * Removes the frame after prevfrn from the queue because this is
 * singly linked list this is done to provide O(1) removal times.
 * prevfrn - the frame id before the one wanting to be removed
 * e.g: 0 -> 3 -> 5 -> 15 -> 2
 * To remove '5' provide prevfrn = '3'
 */
#define frmq_remove(prevfrn)\
    if(frm_queue[(prevfrn)].frq_next == (prevfrn))\
    {\
        frm_q_head = -1;\
        frm_q_tail = -1;\
    }\
    else\
    {\
        int toRmv = frm_queue[(prevfrn)].frq_next;\
        if(toRmv == frm_q_head)\
        {\
            frm_q_head = frm_queue[toRmv].frq_next;\
        }\
        else if(toRmv == frm_q_tail)\
        {\
            frm_q_tail = frm_queue[toRmv].frq_next;\
        }\
        frm_queue[(prevfrn)].frq_next = frm_queue[toRmv].frq_next;\
        frm_queue[toRmv].frq_next = -1;\
    }

