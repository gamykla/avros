/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   MAIN OS KERNEL CODE WRITTEN IN C
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/kernelC.c,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#include <stdlib.h>
#include "SRAM_map.h"
#include "kernelC.h"
#include "ksyscalls.h"
#include "timer.h"
#include "user_init.h"




/********************************************************************
create_initial_process:
    This function is used by the bootstrapping module to create
the very first process ( user_init() ) which is then jumped to by
the bootstrapping code.
*********************************************************************/
void create_initial_process() {

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // needed to set the active index
    unsigned char* active = (unsigned char*)ACTIVE;

    // get access to static temporary storage bytes
    unsigned char* tmp_1 = (unsigned char*)TMP_1;
    unsigned char* tmp_2 = (unsigned char*)TMP_2;

    // active will be pid=0 (user_init) duhhhh
    *active = 0;

    // set required fields
    PDBLOCK[0].sreg = 0;
    PDBLOCK[0].ppid = 0;
    PDBLOCK[0].priority = PRIO_NORMAL;
    PDBLOCK[0].state = S_ACTIVE;

    // let the kernel know where this guys stack is so that
    // it can be loaded into SPH and SPL
    *tmp_1 = PDBLOCK[0].spl;
    *tmp_2 = PDBLOCK[0].sph;

    // thats it, we're ready to jump to this process now

}// create_initial_process()


/********************************************************************
 Timer Interrupt Handler:
 At each timer interrupt we must scan the process descriptor block,
 searching for sleeping processes. When one is found it is then
 neccesary to decrement its sleep time by the period of the timer interrupt.
 If it is then found that the sleep time for a process has been decremented
 to zero (or less as it may be the case), we must change the process state
 from sleeping to ready, as its wait time has expired, and it is now
 ready to re dispatched.
*********************************************************************/
void K_timer_handler() {

    /* some general purpose ints */
    int i;
    int test;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process -  We wan't to save
    the execution state of active!                     */
    unsigned char* active = (unsigned char*)ACTIVE;

    // get active process state information
    unsigned char* a_spl =  (unsigned char*)A_SPL;
    unsigned char* a_sph =  (unsigned char*)A_SPH;
    unsigned char* a_sreg = (unsigned char*)A_SREG;

    // save the state of active!! :
    PDBLOCK[*active].spl  = *a_spl;  // stack pointer high byte
    PDBLOCK[*active].sph  = *a_sph;  // stack pointer low byte
    PDBLOCK[*active].sreg = *a_sreg; // status register

    /* WE want this timer interrupt handler to Be SUPER QuICK */

    // find processes that are in state = S_SLEEPING and take action
    for ( i = 0; i < MAX_PROCS; i++ ) {

        // we find a sleeping process
        if ( PDBLOCK[i].state == S_SLEEPING ) {

            if ( PDBLOCK[i].waitTime <= SLEEP_PERIOD ) {
                // this process should now be put into ready state
                PDBLOCK[i].waitTime = 0;
                PDBLOCK[i].state = S_READY;
            }
            // test > 0
            else {
                // decrement the waitTime and do nothing else - the process still
                // needs to sleep
                PDBLOCK[i].waitTime = PDBLOCK[i].waitTime - SLEEP_PERIOD;
            }

        }// if PDBLOCK[i].state == S_SLEEPING
    }// loop through all processes


    // We now need to select a new Active process:
    schedule();

    // set the new process up to run on the CPU:
    dispatch();

}// K_timer_handler()



/********************************************************************
 System Call Handler:
 We enter this function after a process performs a "trap" to the OS
 by way of INT4. The ASM trap handler will clal this function which
 will then perform the system call by switching on an opcode provided
 by the callee.
*********************************************************************/
void K_syscall() {
    int return_code;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    // get the system call opcode
    unsigned char* opcode = (unsigned char*)OPCODE;

    // get the parameters for system call
    int parm1 = *((int*)P1_L);
    int parm2 = *((int*)P2_L);
    int parm3 = *((int*)P3_L);
    int parm4 = *((int*)P4_L);
    int parm5 = *((int*)P5_L);

    // get active process state information
    unsigned char* a_spl =  (unsigned char*)A_SPL;
    unsigned char* a_sph =  (unsigned char*)A_SPH;
    unsigned char* a_sreg = (unsigned char*)A_SREG;

    // save the state of active:
    PDBLOCK[*active].spl  = *a_spl;  // stack pointer high byte
    PDBLOCK[*active].sph  = *a_sph;  // stack pointer low byte
    PDBLOCK[*active].sreg = *a_sreg; // status register

    // send message to status portA
    asm("ldi r31, 27");
    asm("out 0x1b, r31");


    //*((unsigned int*)TMP_5) = parm1; //////////////////////////////////// TESTING

    // determine which system call we are to perform
    switch ( *opcode ) {
        case GET_PPID      : return_code = K_get_ppid(); break;  // no parms
        case GET_PID       : return_code = K_get_pid(); break;  // no parms
        case GET_PRIO      : return_code = K_get_prio(); break; // no parms
        case SET_PRIO      : return_code = K_set_prio( (unsigned int)parm1, parm2); break; // parm1=pid, parm2=prio
        case SLEEP         : return_code = K_sleep( (unsigned int)parm1 ); break; // parm1=milliseconds
        case KILL          : return_code = K_kill( (unsigned int)parm1); break; // parm1=pid
        case YIELD         : return_code = K_yield() ; break;    // no parms
        case SUSPEND       : return_code = K_suspend( (unsigned int)parm1 ); break; // parm1=pid
        case WAKEUP        : return_code = K_wakeup( (unsigned int)parm1 ); break;  // parm1=pid
        case CREATE_PROCESS: return_code = K_create_process( (unsigned int)parm1, parm2 ); break;  // parm1=program counter parm2=priority
        case SND_MSG       : return_code = K_snd_msg( (unsigned int)parm1, (unsigned char)parm2); break; //parm1=pid parm2=msg
        case CHK_MSG       : return_code = K_check_msg(); break; // no parms
        case GET_MSG       : return_code = K_get_msg(); break; // no parms
        case QUIT          : K_quit(); break;
    } // switch on opcode

    // move return code into Active's process descriptor
    if ( *opcode != QUIT )
        PDBLOCK[*active].returnCode = return_code;

    // TESTING    - works.. as expected
    //*((char*)TMP_3) = return_code;

    // We now need to select a new Active process:
    schedule();

    // prepare the new process to run on the cpu:
    dispatch();


}// K_syscall()


/********************************************************************
 Schedule the execution of a process.
 I.e. Select an Active process

 This simple scheduler is O(n).. Obviously not
 a huge issue since an AVR can't run a thousand
 processes!

 PDBLOCK:
--------
[ pid=0 ][ pid=1 ].....[ pid=MAX_PROCS-1 ]

Policy:
-------
     - We want to choose the process which comes AFTER
     the active process which has MAXIMUM (largest integer)
     priority

*********************************************************************/
void schedule() {

    // send message to status portA
    asm("ldi r31, 28");
    asm("out 0x1b, r31");

    unsigned char i, new_active;
    unsigned char prio_high;
    char found_new;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // set the current active process to ready state
    PDBLOCK[*active].state = S_READY;

    // here we need to determine what the maximum priority is
    // note that it must also belong to a runnable process
    prio_high = PRIO_LOW;
    for ( i = 0; i < MAX_PROCS; i++ ) {
        if ( PDBLOCK[i].priority > prio_high &&  PDBLOCK[i].state == S_READY )
            prio_high = PDBLOCK[i].priority;
    }

    i = (*active)+1;
    if ( i == MAX_PROCS )         // We don't have easy access to %  :(
        i = 0;

    // we will continue until we find a process that is schedulable.
    found_new = 0;
    while ( !found_new ) {

        // condition for finding a new active process
        if ( PDBLOCK[i].priority == prio_high &&
             PDBLOCK[i].state == S_READY ) {

            found_new = 1;
            new_active = i;
        }
        else {
            i++;
            if ( i == MAX_PROCS )
                i = 0;
        }
    }// while (!found_new)

    // the new active now needs to be dispatched:
    *active = new_active;


}// schedule()



/************************************************************************
 Set up the environment for process to execute:
 The active process was selected in the function schedule().
 Before we return to the user execution environment exiting from OS mode
 we need to prepare a process for execution as is done in this function.

     // if PDBLOCK[active].returnCode != NO_RETURN_CODE
     // then signal interrupt handler to move
     // return code into appropriate registers.

     // if there is a return code, must set location RETCODEI to 0x01
     // else set location RETCODEI to 0x00

     // when setting RETCODEI to 0x01 must move PDBLOCK[active].returncode
     // into memory location RETURN_L
*************************************************************************/

void dispatch() {

    // send message to status portA
    asm("ldi r31, 29");
    asm("out 0x1b, r31");

    // We'll store active's data in these locations before
    // setting it off for execution
    unsigned char* a_spl =  (unsigned char*)A_SPL;
    unsigned char* a_sph =  (unsigned char*)A_SPH;
    unsigned char* a_sreg = (unsigned char*)A_SREG;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // the return code indicator in kernel memory
    char* retcodei = (char*)(RETCODEI);

    // the active processes return code;
    int retval = PDBLOCK[*active].returnCode;

    // if there is no return value for this process
    // mem loc. RETCODEI==0 ==> No return value
    if ( retval == NO_RETURN_CODE ) {
        //*(char*)TMP_4) = 0; - debug

        // tell the handler that there is no return value
        *retcodei = NO_RETVAL_FOR_HANDLER;
    }

    // there is a return code
    else {
        //*((char*)TMP_4) = 1; - debug

        // set it back to no return code
        PDBLOCK[*active].returnCode = NO_RETURN_CODE;

        // tell the ASM handler there is a return code
        *retcodei = RETVAL_FOR_HANDLER;

        // place the return code inside the appropriate mem loc.
        *((int*)RETURN_L) = retval;
    }

    // schedule should have selected a new active process.
    // now we must set its state appropriatly.
    PDBLOCK[*active].state = S_ACTIVE;

    // ******************************************************
    // prepare the new active process for execution
    // ******************************************************
    *a_spl = PDBLOCK[*active].spl;
    *a_sph = PDBLOCK[*active].sph;
    *a_sreg = PDBLOCK[*active].sreg;

}// dispatch()



/************************************************************************
 Initialize the Process Descriptor Block:
 This function should be called during system bootstrapping.
 It will initialize the memory are reserved for the process descriptor
 block giving each pd slot its default initialization values.
*************************************************************************/
void K_init_processDescriptorBlock() {

    unsigned int i,j;
    unsigned int sp;
    unsigned char* sp_array;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* Initialize the Process Descriptor Block */
    for ( i = 0; i < MAX_PROCS; i++ ) {
        PDBLOCK[i].pch  = 0;
        PDBLOCK[i].pcl  = 0;

        // get the initial stack pointer
        // ( based on constraints of stackspace size and
        // number of allowable processes )
        sp = K_getInitSP(i);
        sp_array = (unsigned char*)&sp;

        // break up SP into low and high bytes
        PDBLOCK[i].spl  =   sp_array[0];
        PDBLOCK[i].sph  =   sp_array[1];

        PDBLOCK[i].sreg = 0;
        PDBLOCK[i].ppid = -1;
        PDBLOCK[i].pid  = (unsigned char)i;
        PDBLOCK[i].priority = PRIO_NORMAL;
        PDBLOCK[i].state = S_DEAD;

        // process message index
        PDBLOCK[i].msgi = -1;

        /* initialize message buffer */
        for ( j = 0; j < MAX_MESSAGES; j++ )
            PDBLOCK[i].msgbuf[j] = 0;

        PDBLOCK[i].waitTime = 0;
        PDBLOCK[i].returnCode = NO_RETURN_CODE;
    }

    // set active index to first available PID
    *active = 0;
}//K_init_processDescriptorBlock



/**************************************************************************
 Return the initial stack pointer for pid:
 Each process descriptor has a portion of memory reserved for its run time
 stack. This function simply returns the location of the start of the processes
 runtime stack which is usefull for initializing a process prior to creating
 one.
***************************************************************************/
int K_getInitSP( unsigned int pid ) {

    // send message to status portA
    asm("ldi r31, 30");
    asm("out 0x1b, r31");

    return USER_STACKSPACE_END - pid*STACK_SIZE;

}//K_getInitSP










