/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   CPU TIMER STUFF....
;   Warning: This file's data should be synchronized with the
;   information contained within timer.s
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/timer.h,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************/

#ifndef _TIMER_H_
#define _TIMER_H_

// be sure this value is correct with respect to the
// preload value given to timer counter 0.. IF NOT, you'll
// have innacurate sleep times and things can go haywire if you're
// planning on doing anything semi-serious

// number of interrupts per second.
// at this setting we should be getting 10 interrupts per second meaning
// the period is 100 milliseconds
#define HZ 10

#define SLEEP_PERIOD 1000/HZ


#endif

