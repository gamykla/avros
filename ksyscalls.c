/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   SYSTEM CALLS - KERNEL IMPLEMENTATION
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/ksyscalls.c,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#include "kernelC.h"
#include "SRAM_map.h"
#include "ksyscalls.h"
#include "errors.h"


/*************************************************************
K_get_ppid():
    Get the process descriptor ID of the parent of the current process and
    return it to the caller.
**************************************************************/
int K_get_ppid() {

    // send message to status portA
    asm("ldi r31, 31");
    asm("out 0x1b, r31");

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    return PDBLOCK[*active].ppid;

}// K_get_ppid()


/*************************************************************
K_get_pid():
   Get the process descriptor ID of the current process and
   return it to the caller.
**************************************************************/
int K_get_pid() {

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    return 1; PDBLOCK[*active].pid;

}// K_get_pid()


/*************************************************************
K_get_prio():
    Check Active's PD for its priority and return it
**************************************************************/
int K_get_prio() {

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    return PDBLOCK[*active].priority;

}//K_get_prio()


/*************************************************************
K_set_prio():
    Give the active process a new effective priority
**************************************************************/

int K_set_prio(unsigned int pid, int prio) {

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // error if the priority is out of bounds
    if ( prio < PRIO_LOW || prio > PRIO_HIGH )
        return PRIORITY_OUT_OF_RANGE_ERROR;

    // process ID is out of range
    if ( pid >= MAX_PROCS )
        return PID_OUT_OF_RANGE_ERROR;

    // if the process associated with pid isn't alive:
    if ( PDBLOCK[pid].state == S_DEAD )
        return PROCESS_DOES_NOT_EXIST_ERROR;

    // set the processes new priority:
    PDBLOCK[pid].priority = prio;

    return 0;

}//K_set_prio()




/*************************************************************
K_sleep():
    Set active to sleep for sleep_time milliseconds
**************************************************************/
int K_sleep(unsigned int sleep_time) {

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    // change the active processes state to sleeping
    // Now it won't be selected for execution during schedule()
    PDBLOCK[*active].state = S_SLEEPING;

    // set the sleep time in milliseconds. The process will wake up
    // after this much time elapses.
    PDBLOCK[*active].waitTime = sleep_time;

    return 0;

}//K_sleep()




/*************************************************************
K_kill():
    Kill the process pointed to by pid
    cannot kill self
**************************************************************/
int K_kill(unsigned int pid) {

    unsigned int sp, j;
    unsigned char* sp_array;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // a process can't call this function to kill itself.
    // it must call quit() instead
    if ( *active == (unsigned char)pid )
        return CANT_KILL_SELF_ERROR;

    // check that pid is in bounds
    if ( pid > MAX_PROCS-1 )
        return PID_OUT_OF_RANGE_ERROR;

    // if this process pointed to by pid doesn't exist
    if ( PDBLOCK[pid].state == S_DEAD )
        return PROCESS_DOES_NOT_EXIST_ERROR;


    // we must re-initialize the pd:
    PDBLOCK[pid].pch  = 0;
    PDBLOCK[pid].pcl  = 0;

    sp = K_getInitSP(pid);
    sp_array = (unsigned char*)&sp;

    // break up SP into low and high bytes
    PDBLOCK[pid].spl  =   sp_array[0];
    PDBLOCK[pid].sph  =   sp_array[1];

    PDBLOCK[pid].sreg = 0;
    PDBLOCK[pid].ppid = -1;
    PDBLOCK[pid].pid  = pid;
    PDBLOCK[pid].priority = PRIO_NORMAL;
    PDBLOCK[pid].state = S_DEAD;

    // process message index
    PDBLOCK[pid].msgi = -1;

    /* initialize message buffer */
    for ( j = 0; j < MAX_MESSAGES; j++ )
        PDBLOCK[pid].msgbuf[j] = 0;

    PDBLOCK[pid].waitTime = 0;
    PDBLOCK[pid].returnCode = NO_RETURN_CODE;
    
    return 0;

}// K_kill()


/*************************************************************
K_yield():
    Skip over active in the scheduler and allow another
    ready process to execute in place of it.
**************************************************************/
int K_yield() {

    // always return success. The schedule() function will
    // simply skip over active next time and pick a new process
    return 0;
}


/*************************************************************
K_suspend():
    Suspend the process indexed by pid indefinately. That process
    will be in a suspended/blocked mode until another process
    decides to wake it.
**************************************************************/
int K_suspend(unsigned int pid) {

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // a process can't call this function to suspend itself
    // it must call quit() instead
    if ( *active == (unsigned char)pid )
        return CANT_SUSPEND_SELF_ERROR;

    // check that pid is in bounds
    if ( pid > MAX_PROCS-1 )
        return PID_OUT_OF_RANGE_ERROR;

    // if this process pointed to by pid doesn't exist
    if ( PDBLOCK[pid].state == S_DEAD )
        return PROCESS_DOES_NOT_EXIST_ERROR;
        
    // error if pid is already blocked
    if ( PDBLOCK[pid].state == S_BLOCKED )
        return PROCESS_ALREADY_BLOCKED_ERROR;

    // error if pid is sleeping
    if ( PDBLOCK[pid].state == S_SLEEPING )
        return PROCESS_IS_SLEEPING_ERROR;

    PDBLOCK[pid].state = S_BLOCKED;
    
    return 0;

}// K_suspend()


/*************************************************************
K_wakeup():
    Remove the process pointed to by pid from a blocked state
    so as to enable it for selection by the scheduler.
**************************************************************/
int K_wakeup(unsigned int pid) {

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // a process can't call this function to suspend itself
    // it must call quit() instead
    if ( *active == (unsigned char)pid )
        return CANT_WAKE_SELF_ERROR;

    // check that pid is in bounds
    if ( pid > MAX_PROCS-1 )
        return PID_OUT_OF_RANGE_ERROR;

    // if this process pointed to by pid doesn't exist
    if ( PDBLOCK[pid].state == S_DEAD )
        return PROCESS_DOES_NOT_EXIST_ERROR;
        
    if ( PDBLOCK[pid].state != S_BLOCKED )
        return PROCESS_NOT_BLOCKED_ERROR;

    PDBLOCK[pid].state = S_READY;

    return 0;

}// K_wakeup()


/*************************************************************
K_create_process():
    Create a new process if the resources are available.

note that on creating a new process, it is neccessary
to push 0x00 onto the process stack for every register
so that the initial dispatch is handled correctly
by one of the interrupt handlers.

**************************************************************/
int K_create_process(unsigned int pc,  int priority) {

    // send message to status portA
    asm("ldi r31, 40");
    asm("out 0x1b, r31");

    int i,x;
    unsigned char* j;

    unsigned int process_stack_address;
    unsigned char* pstack = (unsigned char*)&process_stack_address;

    unsigned char* physical_stack;

    /* index of the active process - this is required
    since we need to determine ppid  */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    //*((unsigned int*)TMP_7) = 43981;
    //*((unsigned char*)TMP_5) = ((unsigned char*)TMP_7)[0];  ////////////TESTING//////////////////
    //*((unsigned char*)TMP_6) = ((unsigned char*)TMP_7)[1];


    // just check up on the priority levels
    if ( priority < PRIO_LOW || priority > PRIO_HIGH ) {
        //*((char*)TMP_3) = 1;              /// ERROR CHECKING
        return PRIORITY_OUT_OF_RANGE_ERROR;
    }

    // find first available free PDBLOCK area
    for ( i = 0; i < MAX_PROCS; i++ ) {
        if ( PDBLOCK[i].state == S_DEAD )
            break;
    }

    // nothing is available
    if ( i == MAX_PROCS ) {
        //*((char*)TMP_3) = 2;            /// ERROR CHECKING
        return NO_FREE_PROCESS_SLOTS_AVAIL_ERROR;
    }

    // we have found a free process at index i
    // setup the pidblock

    PDBLOCK[i].ppid = *active;       // obvious.. active is parent

    // store the pc in the pidblock
    j = (unsigned char*)&pc;
    PDBLOCK[i].pcl = (unsigned char)j[0];
    PDBLOCK[i].pch = (unsigned char)j[1];

    // set priority
    PDBLOCK[i].priority = priority;

    // ready the process
    PDBLOCK[i].state = S_READY;

    // now we have to prepare the stack and reset PDBLOCK[i].sph and
    // the low byte accordingly! (in order for dispatch to occur cleanly!
    // see kernel.s)

    pstack[0] = (unsigned char)PDBLOCK[i].spl;
    pstack[1] = (unsigned char)PDBLOCK[i].sph;

    // get a pointer to the stack;
    physical_stack = (unsigned char*)process_stack_address;

    // throw the return address onto this processes stack
    // so that the interrupt handler will correctly dispatch
    // this process
    *physical_stack = PDBLOCK[i].pcl;
    physical_stack--;
    *physical_stack = PDBLOCK[i].pch;
    physical_stack--;

    // zero the 32 registers (on the stack)
    for ( x = 0; x < 32; x++ ) {
        *physical_stack = (unsigned char)0;
         physical_stack--;
    }

    // insert the updated stack pointer into the process
    // descriptor
    process_stack_address = (unsigned int)physical_stack;
    PDBLOCK[i].spl = pstack[0];
    PDBLOCK[i].sph = pstack[1];

    // move the new stack pointer into the new pidblock location
    // return the pid of the newly created process to active
    // THE NEW PROCESS IS NOW READY FOR EXECUTION
    return i;

}//K_create_process()


/*************************************************************
K_snd_msg():
    Send a message to the process indexed by pid and have the
    sender be designated as the current active process pid
**************************************************************/
int K_snd_msg(unsigned int pid, unsigned char msg) {

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    unsigned char pid_of_origin = *active;
    int message;   // the message to be sent
    unsigned char i;

    int* msg_a = &message;

    // check that pid is in bounds
    if ( pid > MAX_PROCS-1 )
        return PID_OUT_OF_RANGE_ERROR;

    // if this process pointed to by pid doesn't exist
    if ( PDBLOCK[pid].state == S_DEAD )
        return PROCESS_DOES_NOT_EXIST_ERROR;

    // now we are not permitted to overflow pid's message stack
    if ( PDBLOCK[pid].msgi == MAX_MESSAGES-1 )
        return FULL_MESSAGE_STACK_ERROR;

    // allright, so now it looks like we can safely add
    // the message:

    // process of origin
    msg_a[0] = pid_of_origin;
    // message content
    msg_a[1] = msg;

    PDBLOCK[pid].msgi++;
    i = PDBLOCK[pid].msgi;

    PDBLOCK[pid].msgbuf[i] = message;

    return 0;

}//K_snd_msg()


/*************************************************************
K_check_msg():
    Tell Active if there are any pending messages waiting to
    be retrieved.

    return 1 => There are waiting messages
    return 0 => There are no waiting messages

**************************************************************/
int K_check_msg() {

    unsigned char* active = (unsigned char*)ACTIVE;
    unsigned int pid = (unsigned int)*active;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // there are pending messages
    if ( PDBLOCK[pid].msgi != -1 )
        return 1;

    // no messages
    return 0;

}//K_check_msg()



//*******************************************
// NEED A FUNC. HERE TO GET OLDEST MESSAGE
// ( I.e. one at the bottom of the stack!)
//
//  Low Prio...
//
//*******************************************




/*************************************************************
K_get_msg():
    Return a message to the active process if one is pending
    inside its message stack

    The message which is returned in this function is that
    which is at the top of the message stack
**************************************************************/
int K_get_msg() {

    unsigned char* active = (unsigned char*)ACTIVE;
    unsigned int pid = (unsigned int)*active;

    int currentMsgI;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // return an error if there are no messages pending
    currentMsgI = (int)PDBLOCK[pid].msgi;

    if ( currentMsgI == -1 )
        return NO_PENDING_MSGS_ERROR;

    // else we return a message and decrement
    currentMsgI -= 1;
    PDBLOCK[pid].msgi = (char)currentMsgI;

    return PDBLOCK[pid].msgbuf[currentMsgI+1];

}//K_get_msg()


/*************************************************************
K_quit():
    Kill the active process and free its resources so that
    other process may be created.
**************************************************************/
void K_quit() {

    unsigned int sp, j;
    unsigned char* sp_array;
    unsigned int pid;

    /* index of the active process */
    unsigned char* active = (unsigned char*)ACTIVE;

    pid = (unsigned int)*active;

    /* get pointer to process descriptor block */
    struct PD* PDBLOCK = (struct PD*)PD_BLOCK_START;

    // we must re-initialize the pd:
    PDBLOCK[pid].pch  = 0;
    PDBLOCK[pid].pcl  = 0;

    sp = K_getInitSP(pid);
    sp_array = (unsigned char*)&sp;

    // break up SP into low and high bytes
    PDBLOCK[pid].spl  =   sp_array[0];
    PDBLOCK[pid].sph  =   sp_array[1];

    PDBLOCK[pid].sreg = 0;
    PDBLOCK[pid].ppid = -1;
    PDBLOCK[pid].pid  = pid;
    PDBLOCK[pid].priority = PRIO_NORMAL;
    PDBLOCK[pid].state = S_DEAD;

    // process message index
    PDBLOCK[pid].msgi = -1;

    /* initialize message buffer */
    for ( j = 0; j < MAX_MESSAGES; j++ )
        PDBLOCK[pid].msgbuf[j] = 0;

    PDBLOCK[pid].waitTime = 0;
    PDBLOCK[pid].returnCode = NO_RETURN_CODE;

}// K_Quit()


