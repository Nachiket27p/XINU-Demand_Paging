/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
    popl pferrcode          /* pop the ref code of the stack            */
    pushfl                  /* push the f register onto the stack       */
    cli                     /* clear the interrupt flag                 */
    pushal                  /* push the a register onto the stack       */
    call pfint              /* call the pfint() function                */
    popal                   /* restore original reg a value from stack  */
    popfl                   /* restore original reg f value from stack  */
    iret                    /* return from this interrupt subroutine    */

