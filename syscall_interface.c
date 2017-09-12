/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  USER SYSTEM CALL INTERFACE AND FUNCTIONS
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/syscall_interface.c,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#include "SRAM_map.h"
#include "kernelC.h"
#include "syscall_interface.h"


/***********************************************************************
 syscall(): This function performs a call to the operating system in
 order to carry out a processes system call. It does so by loading the
 OS memory region with required parameters and a syscall identifier/Opcode
 and then triggering an interrupt in order that the operating system
 save all context and processes the requested call.

 The definition of this function allows for a maximum of 5 syscall
 parameters to be defined in the case of a future need of expansion.
************************************************************************/
int syscall( unsigned char opcode, int p1, int p2, int p3, int p4, int p5 ) {

    /* get a pointer to the global opcode location in memory*/
    unsigned char* opcode_ptr = (unsigned char*)OPCODE;

    /* get pointers to the global locations of parameters for syscalls */
    int* parm1_ptr = (int*)P1_L;
    int* parm2_ptr = (int*)P2_L;
    int* parm3_ptr = (int*)P3_L;
    int* parm4_ptr = (int*)P4_L;
    int* parm5_ptr = (int*)P5_L;

    int ret_code = -1;  /* return value for system call */

    // send message to status portA
    asm("ldi r31, 11");
    asm("out 0x1b, r31");

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // CRITICAL SECTION START:
    // we can't permit another process to start inside here,
    // as it could result in a corruption of the system call parameters
    // being passed to kernel memory.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // stop timer counter 0 since we don't want a context switch
    // while we're preping the syscall.
    // WARNING: These asm statements are specific to ATmega128 architecture.
    asm("cli");
    asm("ldi r16, 0b000");
    asm("out 0x33,r16"); // 0x33 = TCCR0, on the ATmega128
    asm("sei");

    // timer is now stopped => timer interrupts disabled!

    //*((unsigned int*)TMP_5) = p1;     /////////////////////////////////////////// TESTING

    // load parameters and opcode into kernel memory:
    *opcode_ptr = opcode;
    *parm1_ptr = p1;
    *parm2_ptr = p2;
    *parm3_ptr = p3;
    *parm4_ptr = p4;
    *parm5_ptr = p5;

        // send message to status portA
    asm("ldi r31, 12");
    asm("out 0x1b, r31");

    //*((unsigned int*)TMP_5) = *parm1_ptr;   /////////////////////////////////////////// TESTING

    /* Prepare a trap to the operating system */
    asm("ldi r16, 0x00");
    asm("out 0x03, r16"); // send data 0b00000000 to PORTE(0x03 on ATmega128)
                          // this will trigger INT4 to occur which
                          // is pinned on PORTE during OS initialization.

    asm("nop");           // we'll stall for a while in order to allow
    asm("nop");           // INT4 to trigger
    asm("nop");
    asm("nop");           // at this point, the OS should begin at the
    asm("nop");           // INT4 Handler.
    asm("nop");

    // get return code from kernel memory
    // NOTE: Its impossible that the process be interrupted at this point due to the
    // cpu cycle bonus given to each dispatched process.
    ret_code  =  *((int*)RETURN_L);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // CRITICAL SECTION END
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    return ret_code;

} //syscall()

// ***************************************************************
// get_pid():
// Description: Get process ID for currently executing task.
// Return: Returns the pid of the currently executing process.
// ***************************************************************
int get_pid() {

    // send message to status portA
    asm("ldi r31, 14");
    asm("out 0x1b, r31");

    return syscall(GET_PID, 0, 0, 0, 0, 0);

}

// ***************************************************************
// get_ppid():
// Description: Get parent process ID
// Return: Returns the pid of the parent process.
//         If called from within user_init(), ppid=0.
// ***************************************************************
int get_ppid() {

    // send message to status portA
    //asm("ldi r31, 13");
    //asm("out 0x1b, r31");

    return syscall(GET_PPID,0,0,0,0,0);
}



// ***************************************************************
// get_prio():
// Description: Used to get the priority level of the currently
// executing process.
// Return: Returns an integer value x, st. PRIO_LOW <= x <= PRIO_HIGH
// ***************************************************************
int get_prio() {

    // send message to status portA
    //asm("ldi r31, 15");
    //asm("out 0x1b, r31");

    return syscall(GET_PRIO, 0, 0, 0, 0, 0);
}


// ***************************************************************
// set_prio():
// Description: set priority for pid
// Parms: pid  -> process id for which priority is to be set.
//        prio -> the new priority for process pid.
//        PRIO_LOW <= prio <= PRIO_HIGH
// Return: 0 on sucess, -1 on error.
// ***************************************************************
int set_prio(unsigned int pid, int prio) {

    // send message to status portA
    //asm("ldi r31, 16");
    //asm("out 0x1b, r31");

    return syscall(SET_PRIO, pid, prio, 0, 0, 0);

}


// ***************************************************************
// sleep():
// Description: Sleep for a certain number of milliseconds.
// The calling process shall be suspended untill sleep_time MS passes.
// Parms: sleep_time -> long integer number of milliseconds to sleep.
// Return: 0 on success, -1 on error.
// ***************************************************************
int sleep(unsigned int sleep_time) {

    // send message to status portA
    //asm("ldi r31, 17");
    //asm("out 0x1b, r31");

    return syscall(SLEEP, sleep_time, 0, 0, 0, 0);

}


// ***************************************************************
// kill():
// Description: kill a process with process id pid
// ***************************************************************
int kill(unsigned int pid) {

    // send message to status portA
    //asm("ldi r31, 18");
    //asm("out 0x1b, r31");

    return syscall(KILL, pid, 0, 0, 0, 0);

}


// ***************************************************************
// yield():
// Description: yield control of the cpu to another process
// ***************************************************************
int yield() {

    // send message to status portA
    //asm("ldi r31, 19");
    //asm("out 0x1b, r31");

    return syscall(YIELD, 0, 0, 0, 0, 0);

}


// ***************************************************************
// suspend():
// Description: suspend the execution of another process
// ***************************************************************
int suspend(unsigned int pid) {

    // send message to status portA
    //asm("ldi r31, 20");
    //asm("out 0x1b, r31");

    return syscall(SUSPEND, pid, 0, 0, 0, 0);

}


// ***************************************************************
// wakeup():
// Description: wakeup a suspended process
// ***************************************************************
int wakeup(unsigned int pid) {

    // send message to status portA
    //asm("ldi r31, 21");
    //asm("out 0x1b, r31");

    return syscall(WAKEUP, pid, 0, 0, 0, 0);

}


// ***************************************************************
// create_process():
// Description: create a new process
// ***************************************************************
int create_process(unsigned int pc, int prio) {

    // send message to status portA
    asm("ldi r31, 22");
    asm("out 0x1b, r31");

    *((unsigned int*)TMP_5) = pc;

    return syscall(CREATE_PROCESS, pc, prio, 0, 0, 0);

}


// ***************************************************************
// send_msg():
// Description: send a message to a process
// ***************************************************************
int snd_msg(unsigned int pid, unsigned char msg) {

    // send message to status portA
    //asm("ldi r31, 23");
    //asm("out 0x1b, r31");

    return syscall(SND_MSG, pid, msg, 0, 0, 0);

}


// ***************************************************************
// check_msg():
// Description: check if there are any pending messages
// ***************************************************************
int check_msg() {

    // send message to status portA
    //asm("ldi r31, 24");
    //asm("out 0x1b, r31");

    return syscall(CHK_MSG, 0, 0, 0, 0, 0);

}


// ***************************************************************
// get_msg():
// Description: get a message
// ***************************************************************
int get_msg() {

    // send message to status portA
    //asm("ldi r31, 25");
    //asm("out 0x1b, r31");

    return syscall(GET_MSG, 0, 0, 0, 0, 0);

}


// ***************************************************************
// quit():
// Description: terminate self
// ***************************************************************
void quit() {

    // send message to status portA
    //asm("ldi r31, 26");
    //asm("out 0x1b, r31");

    syscall(QUIT,0,0,0,0,0);
    // no rerturn val => we're NEVER going to make it here...

}



