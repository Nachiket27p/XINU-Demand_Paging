#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>
#include <frm_q.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 100);

	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	xmunmap(PROC1_VPNO);
	return;
}

void proc1_test2(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	*(x + 1) = 200;

	kprintf("heap variable: %d %d\n", *x, *(x + 1));
	vfreemem(x, 1024);
}

void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}
int SC_next_victim;
void printfrmq()
{
	if(frmq_isEmpty())
	{
		kprintf("\nQueue is Empty!\n\n");
		return;
	}

	kprintf("head: %d\n", frm_q_head);
	kprintf("tail: %d\n", frm_q_tail);

	int curr = frm_q_head;
	while(curr != frm_q_tail)
	{
		kprintf("%d --> ", curr);
		curr = frmq_next_id(curr);
	}
	kprintf("%d --> ", curr);
	kprintf("%d\n\n", frmq_next_id(curr));
}

void test_frmq()
{
	printfrmq();

	frmq_insert(15);

	printfrmq();

	frmq_insert(20);
	
	printfrmq();

	frmq_insert(210);

	printfrmq();

	frmq_insert(270);

	printfrmq();

	frmq_remove(frm_q_head);

	printfrmq();

	frmq_remove(frm_q_tail);

	printfrmq();

	frmq_remove(frm_q_head);

	printfrmq();

	frmq_remove(frm_q_head);

	printfrmq();
}

int main()
{

	test_frmq();

	// int pid1;
	// int pid2;

	// kprintf("\n1: shared memory\n");
	// pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	// resume(pid1);
	// sleep(10);

	// kprintf("\n2: vgetmem/vfreemem\n");
	// pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
	// kprintf("pid %d has private heap\n", pid1);
	// resume(pid1);
	// sleep(3);

	// kprintf("\n3: Frame test\n");
	// pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	// resume(pid1);
	// sleep(3);

	return 0;
}
