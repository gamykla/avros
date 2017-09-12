/****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   USER PROGRAM INITIALIZATION CODE
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/user_init.c,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 01:58:06 $
;  $Revision: 1.2 $
;
;****************************************************************/


#include "SRAM_map.h"
#include "errors.h"
#include "syscall_interface.h"
#include "kernelC.h"

void process_2();
void process_3();
void process_4();


/******************************************
 This thread will  always be PID=0. It should create all
 other process by way of system calls and whatever else you
 fancy.
********************************************/
void user_init() {
    // send message to status portA
    asm("ldi r31, 0");
    asm("out 0x1b, r31");

     int i;
     int child;

    *((unsigned int*)TMP_5) = (unsigned int)&process_2;

    child = create_process((unsigned int)&process_2, PRIO_NORMAL);

    set_prio(1, PRIO_LOW);
    set_prio(2, PRIO_LOW);
    set_prio(3, PRIO_LOW);


    while ( 1 ) {

       *((char*)TMP_5) = i++;
    }

    return;
}


// a scecond process -
// play with mem loc: TMP_6
void process_2() {
    int i = 0;
    int child;

    child = create_process((unsigned int)&process_3, PRIO_NORMAL);

    while (1) {
       *((char*)TMP_6) = i++;
    }

}


// a scecond process -
// play with mem loc: TMP_6
void process_3() {
    int i = 0;

    int child;
    child = create_process((unsigned int)&process_4, PRIO_NORMAL);

    while (1) {
       *((char*)TMP_7) = i++;
    }

}

void process_4() {
    int i = 0;


    while (1) {
       *((char*)TMP_8) = i++;
    }

}










