/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   MAIN OS KERNEL CODE WRITTEN IN C
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/kernelC.h,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#ifndef _KERNELC_H_
#define _KERNELC_H_


// return value for PD if there is nothing to return
// this definition is neccesary due to the fact that certain syscalls
// have no return value. After each system call, memory location RETURN_L
// will be checked for a value. If it doesn't equal the value below, it is
// an indicator that the system call returned a value.
#define NO_RETURN_CODE -100

// settings for kenel memory location RETCODEI
#define NO_RETVAL_FOR_HANDLER   0
#define RETVAL_FOR_HANDLER      1

// Maximum number of processes:
// This value can be tweaked. It is currently set under the assumption
// that we're operating in an architecture such as the ATmega128 with
// only base on chip RAM installed. I.e. if we had access to external
// ram (up to 65k), then it would be acceptable to increase this value.

// do not exceed 6
#define MAX_PROCS 6

// process priority levels:
// These values can be tweaked to suit the needs of users. They
// are simply provided here as defaults.
#define PRIO_LOW    0  // minimum priority in system
#define PRIO_NORMAL 3
#define PRIO_HIGH   6  // maximum priority in system.

// message buffer size:
#define MAX_MESSAGES 10  // a process may have up to this many pending messages.

// stack size per process:
// obviously dependant on the number of allowable processes and memory available.
#define STACK_SIZE ((USER_STACKSPACE_END - USER_STACKSPACE_START)/MAX_PROCS)

// System Call Operation Codes:
#define GET_PPID          0
#define GET_PID           1
#define GET_PRIO          2
#define SET_PRIO          3
#define SLEEP             4
#define KILL              5
#define YIELD             6
#define SUSPEND           7
#define WAKEUP            8
#define CREATE_PROCESS    9
#define SND_MSG           10
#define CHK_MSG           11
#define GET_MSG           12
#define QUIT              13

// process states:
#define S_ACTIVE     0  // The process which currently owns the CPU
#define S_DEAD       1  // A process which is not active and will not get the CPU.
#define S_BLOCKED    2  // a process that is waiting for an event to occur.
#define S_SLEEPING   3  // a process that has been put to sleep.
#define S_READY      4  // a process that is waiting on the execution queue.

// Process Descriptor:
struct PD {
    int returnCode;            // return code from system calls
    unsigned char pch;
    unsigned char pcl;
    unsigned char sph;         // stack pointer high
    unsigned char spl;         // stack pointer low
    unsigned char sreg;        // status register

    unsigned char ppid;        // parent process ID
    unsigned char pid;         // process ID
    unsigned char priority;    // process priority
    unsigned char state;       // process state

    // the message buffer will act as a stack growing from msgi=0
    char msgi;                 // message index
    int msgbuf[MAX_MESSAGES];  // message buffer
    unsigned int waitTime;     // sleep time in miliseconds
};

/****************

ABOUT MESSAGES:
===============

the message stack (grows from low msgi to high msgi):
-----------------------------------------------------

msgbuf[0]
msgbuf[1]
msgbuf[2]
msgbuf[3]                           <----- msgi=3
.
.
msgbuf[MAX_MESSAGES-1]

message format (each message is an int consisting of 2 parts)
--------------------------------------------------------------


   A        B          A=pid of origin   ( obviously an unsigned PDBLOCK index )
-------- --------      B=message content ( 0b00000000 to 0b11111111 )
-------- --------
byte 1   byte 2

Note - Byte 1 is the "low" byte and Byte 2 is the "high" byte.
     - A communication protocol should be defined for the application
       at hand.
****************/



// Kernel Functions

/************************************************************************
 Initialize the Process Descriptor Block:
 This function should be called during system bootstrapping.
 It will initialize the memory are reserved for the process descriptor
 block giving each pd slot its default initialization values.
*************************************************************************/
void K_init_processDescriptorBlock();


/**************************************************************************
 Return the initial stack pointer for pid:
 Each process descriptor has a portion of memory reserved for its run time
 stack. This function simply returns the location of the start of the processes
 runtime stack which is usefull for initializing a process prior to creating
 one.
***************************************************************************/
int K_getInitSP( unsigned int pid );

/********************************************************************
 System Call Handler: 
 We enter this function after a process performs a "trap" to the OS
 by way of INT4. The ASM trap handler will clal this function which
 will then perform the system call by switching on an opcode provided
 by the callee.
*********************************************************************/
void K_syscall();

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
void K_timer_handler();

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
void schedule();


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
void dispatch();


/*******************************************************************
 The idle task:
 This task takes over the CPU when there is
 no other work to be done.
********************************************************************/
void K_idle();



/********************************************************************
create_initial_process:
    This function is used by the bootstrapping module to create
the very first process ( user_init() ) which is then jumped to by
the bootstrapping code.
*********************************************************************/
void create_initial_process();


#endif

