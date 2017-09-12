
/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   AVROS ERROR CODES
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/errors.h,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#ifndef _ERRORS_H_
#define _ERRORS_H_

#define PRIORITY_OUT_OF_RANGE_ERROR   -1
#define PID_OUT_OF_RANGE_ERROR        -2
#define CANT_SUSPEND_SELF_ERROR       -3
#define CANT_KILL_SELF_ERROR          -4
#define PROCESS_DOES_NOT_EXIST_ERROR  -5
#define PROCESS_ALREADY_BLOCKED_ERROR -6
#define PROCESS_IS_SLEEPING_ERROR     -7
#define CANT_WAKE_SELF_ERROR          -8
#define PROCESS_NOT_BLOCKED_ERROR     -9
#define NO_PENDING_MSGS_ERROR         -10
#define FULL_MESSAGE_STACK_ERROR      -11
#define NO_FREE_PROCESS_SLOTS_AVAIL_ERROR -12


#endif

