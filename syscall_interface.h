/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  USER SYSTEM CALL INTERFACE
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/syscall_interface.h,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#ifndef _SYSCALL_INTERFACE_H_
#define _SYSCALL_INTERFACE_H_



int syscall( unsigned char opcode, int p1, int p2, int p3, int p4, int p5 );

// process control system calls:

// ***************************************************************
// get_ppid():
// Description: Get parent process ID
// Return: Returns the pid of the parent process.
//         If called from within user_init(), ppid=0.
// ***************************************************************
int get_ppid();

// ***************************************************************
// get_pid():
// Description: Get process ID for currently executing task.
// Return: Returns the pid of the currently executing process.
// ***************************************************************
int get_pid();

// ***************************************************************
// get_prio():
// Description: Used to get the priority level of the currently
// executing process.
// Return: Returns an integer value x, st. PRIO_LOW <= x <= PRIO_HIGH
// ***************************************************************
int get_prio();

// ***************************************************************
// set_prio():
// Description: set priority for pid
// Parms: pid  -> process id for which priority is to be set.
//        prio -> the new priority for process pid.
//        PRIO_LOW <= prio <= PRIO_HIGH
// Return: 0 on sucess, -1 on error.
// ***************************************************************
int set_prio(unsigned int pid,  int prio);

// ***************************************************************
// sleep():
// Description: Sleep for a certain number of milliseconds.
// The calling process shall be suspended untill sleep_time MS passes.
// Parms: sleep_time -> long integer number of milliseconds to sleep.
// Return: 0 on success, -1 on error.
// ***************************************************************
int sleep(unsigned int sleep_time);

// ***************************************************************
// kill():
// Description: kill a process with process id pid
// ***************************************************************
int kill(unsigned int pid);

// ***************************************************************
// yield():
// Description: yield control of the cpu to another process
// ***************************************************************
int yield();

// ***************************************************************
// suspend():
// Description: suspend the execution of another process
// ***************************************************************
int suspend(unsigned int pid);

// ***************************************************************
// wakeup():
// Description: wakeup a suspended process
// ***************************************************************
int wakeup(unsigned int pid);

// ***************************************************************
// create_process():
// Description: create a new process
// ***************************************************************
int create_process(unsigned int pc, int prio);

// ***************************************************************
// send_msg():
// Description: send a message to a process
// ***************************************************************
int snd_msg(unsigned int pid, unsigned char msg);

// ***************************************************************
// check_msg():
// Description: check if there are any pending messages
// ***************************************************************
int check_msg();

// ***************************************************************
// get_msg():
// Description: get a message
// ***************************************************************
int get_msg();

// ***************************************************************
// quit():
// Description: terminate self
// ***************************************************************
void quit();

#endif

