/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  SYSTEM CALLS KERNEL IMPLEMENTATION
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/ksyscalls.h,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#ifndef _KSYSCALLS_H_
#define _KSYSCALLS_H_

// ******************************************************
// System Calls:
// ******************************************************



/*************************************************************
K_get_ppid():
    Get the process descriptor ID of the parent of the current process and
    return it to the caller.
**************************************************************/
int K_get_ppid();


/*************************************************************
K_get_pid():
   Get the process descriptor ID of the current process and
   return it to the caller.
**************************************************************/
int K_get_pid();


/*************************************************************
K_get_prio():
    Check Active's PD for its priority and return it
**************************************************************/
int K_get_prio();


/*************************************************************
K_set_prio():
    Give the active process a new effective priority
**************************************************************/
int K_set_prio(unsigned int pid,  int prio);


/*************************************************************
K_sleep():
    Set active to sleep for sleep_time milliseconds
**************************************************************/
int K_sleep(unsigned int sleep_time);


/*************************************************************
K_kill():
    Kill the process pointed to by pid
    cannot kill self
**************************************************************/
int K_kill(unsigned int pid);


/*************************************************************
K_yield():
    Skip over active in the scheduler and allow another
    ready process to execute in place of it.
**************************************************************/
int K_yield();


/*************************************************************
K_suspend():
    Suspend the process indexed by pid indefinately. That process
    will be in a suspended/blocked mode until another process
    decides to wake it.
**************************************************************/
int K_suspend(unsigned int pid);


/*************************************************************
K_wakeup():
    Remove the process pointed to by pid from a blocked state
    so as to enable it for selection by the scheduler.
**************************************************************/
int K_wakeup(unsigned int pid);


/*************************************************************
K_create_process():
    Create a new process if the resources are available.
**************************************************************/
int K_create_process(unsigned int pc,  int priority);


/*************************************************************
K_snd_msg():
    Send a message to the process indexed by pid and have the
    sender be designated as the current active process pid
**************************************************************/
int K_snd_msg(unsigned int pid, unsigned char msg);

/*************************************************************
K_check_msg():
    Tell Active if there are any pending messages waiting to
    be retrieved.

    return 1 => There are waiting messages
    return 0 => There are no waiting messages

**************************************************************/
int K_check_msg();


/*************************************************************
K_get_msg():
    Return a message to the active process if one is pending
    inside its message stack

    The message which is returned in this function is that
    which is at the top of the message stack
**************************************************************/
int K_get_msg();

/*************************************************************
K_quit():
    Kill the active process and free its resources so that
    other process may be created.
**************************************************************/
void K_quit();


#endif

