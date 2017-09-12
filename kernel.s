;****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  OS BOOTSTRAPPING SECTION AND INTERRUPT HANDLERS
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/kernel.s,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************

; define the architecture type (just for the assembler)
.arch atmega128

; register and i/o alias definitions for the device
.include "ATmega128defs.s"

; SRAM memory map including global data areas
.include "SRAM_map.s"

; timing definitions and timer interrupt stuff
.include "timer.s"

; interrupt vector table (must start at very beginning of PGM memory)
.include "vector_table.s"


;********************************************
; Kernel Initialization
;********************************************
.global main
main:    ; label this section 'main:' to satisfy the final system link
K_init:
    ; Set System Stack Pointer: we put it at the very end/top of SRAM
    ldi r16, lo8(RAMEND)
    out SPL, r16
    ldi r16, hi8(RAMEND)
    out SPH, r16

    ; Status message on PORTA
    ldi r31, 0x01
    out 0x1b, r31

    ; initialize process descriptors
    rcall K_init_processDescriptorBlock

    ; Status message on PORTA
    ldi r31, 0x02
    out 0x1b, r31

    ; *************************************
    ; enable overflow interrupts on timer0:
    ; this functionality is neccesary for round robin pre-emptive
    ; CPU scheduling.
    ; *************************************
    ldi r16,INITIAL_TIMER0_COUNT  ; set initial count
    out TCNT0,r16

    ; enable timer/counter overflow interrupt
    ldi r16,(1<<TOIE0)
    out TIMSK,r16

    ; set clock prescaler in timer/counter0 control reg.
    ldi r16, CPRESCALAR_1024
    out TCCR0,r16

    ; Status message on PORTA
    ldi r31, 0x03
    out 0x1b, r31

    ; *************************************
    ; set up trap to operating system on INT4
    ; Processes will use INT4 to perform system calls.
    ; *************************************
    ldi r16, 0b00010000  ; configure PE4(int4) as output pin
    out DDRE, r16

    ; set PE4(INT4) high : 00010000 (low signal will cause INT4)
    ldi r16, 0b00010000
    out PORTE, r16

    ; Set external interrupt control register to generate INT4 on low level
    ldi r16, 0x00
    out EICRB, r16

    ; set up EIMSK for interrupts on INT4
    ldi r16, 0b00010000
    out EIMSK, r16

    ; Status message on PORTA
    ldi r31, 0x04
    out 0x1b, r31

    ; create the first process (user_init)
    rcall create_initial_process

    ; Status message on PORTA
    ldi r31, 0x05
    out 0x1b, r31

    ; save kernel sp
    in r16, SPH
    in r17, SPL
    sts KSP_HIGH, r16
    sts KSP_LOW, r17


    ; need to load user_init stack pointer:
    ;    tmp_1 = PDBLOCK[0].spl;
    ;    tmp_2 = PDBLOCK[0].sph;
    ; note that TMP_1 and TMP_2 are set by create_initial_process()
    lds r16, TMP_1
    lds r17, TMP_2
    out SPL, r16
    out SPH, r17

    ; enable interrupts
    sei

    ; jump to main user thread:
    ; user_init must be linked into the runtime object.
    ; user_init() acts as an initial user program.
    ; refer to user_init.c and user_init.h
    jmp user_init




;********************************************
; OS Trap (INT4)  Interrupt Handler
;********************************************
K_trap:

    ; save all registers on user stack
    push r0
    push r1
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r18
    push r19
    push r20
    push r21
    push r22
    push r23
    push r24
    push r25
    push r26
    push r27
    push r28
    push r29
    push r30
    push r31

    ; Status message on PORTA
    ldi r31, 45
    out 0x1b, r31

    ; Re-enable INT4
    ldi r16, 0b00010000
    out PORTE, r16

    ; Save Active process Stack Pointer
    in r30, SPH ; move SP into Z
    in r31, SPL

    ;  move the pointers to their ram locations
    ; these will need to be retrieved by other kernel components later!!!
    sts A_SPH,  r30
    sts A_SPL,  r31

    ; Save Active Status Register to ram
    in r18, SREG
    sts A_SREG, r18

    ; ***********************************************************
    ; switch to kernel stack:
    lds r19, KSP_HIGH
    lds r20, KSP_LOW
    out SPH, r19
    out SPL, r20

    ; goto the System Call Handler
    call K_syscall

    ; save the kernel stack pointer back to ram
    in r19, SPH
    in r20, SPL
    sts KSP_HIGH, r19
    sts KSP_LOW,  r20
    ; ***********************************************************

    ; Load new active stack pointer
    lds r19, A_SPL
    lds r20, A_SPH
    out SPL, r19
    out SPH, r20

    ; re-enable timer/counter0
    in r16, TCNT0  ; subtract 1 from the timer (gives a bonus of XXX cycles to the process running next )
    dec r16                                   ; Bonus dependant on value in TCCR0 -> default = 1024
    out TCNT0, r16
    ldi r16, 0b111   ; re-enable the timer
    out TCCR0,r16



    ; check if there is a return value to give the new process
    lds r16, RETCODEI
    ldi r17, 0x00 ; RETCODEI=0  ==> No return value
    cp r16, r17
    breq TRAP_NO_RETVAL   ; there is no return value


    ; Status message on PORTA
    ldi r31, 46
    out 0x1b, r31

    ; restore all registers and save return value:
    ; Load new active status register
    lds r19, A_SREG
    out SREG, r19

    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r18
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    pop r0

    ; save return value - r24:low, r25:high
    lds r24, RETURN_L  ;low      (store int return val by AVR C convention)
    lds r25, RETURN_H  ;high
    rjmp TRAP_EXIT


; why no retval? Could be a new process, could have been interrupted by a timer etc.
TRAP_NO_RETVAL:  ; we jump here if there is no return value
                 ; notice that this is what happens if a process
                 ; is dispatched by the system for the very first
                 ; time, since it won't have any return val (obviously).


    ; Status message on PORTA
    ldi r31, 47
    out 0x1b, r31

    ; Load new active status register
    lds r19, A_SREG  ; if we're loading a new proc, this should be 0x00
    out SREG, r19

    ; restore all registers
    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24      ; The stack for a brand new process must be configured
    pop r23      ; so that it already contains r0-r31 AND the program
    pop r22      ; counter for the first instruction that is executed
    pop r21      ; within the process.
    pop r20
    pop r19
    pop r18
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    pop r0

TRAP_EXIT:

    ; jump to new active process: this command will load the
    ; new program counter of the stack and jump to that section of code.
    reti




;********************************************
; TIMER0 Overflow Interrupt handler:
; invoked as timer/counter0 overflows.
; This code is essentially the same as that of
; the Interrupt 4 code except that in this case its important
; that we modify the timer value so that it works correctly again.
;********************************************
tc0_ovf:
    ; save all registers on user stack
    push r0
    push r1
    push r2
    push r3
    push r4
    push r5         ; any interrupt must behave in the exact same way as this
    push r6         ; one, because after processing an interrupt, the
    push r7         ; fact that we multiplex process execution could result
    push r8         ; in another proces
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r18
    push r19
    push r20
    push r21
    push r22
    push r23
    push r24
    push r25
    push r26
    push r27
    push r28
    push r29
    push r30
    push r31

    ; Save Active Stack Pointer
    in r30, SPH ; move SP into Z
    in r31, SPL
    sts A_SPH,  r30    ; move SP to memory
    sts A_SPL,  r31
    ; these will need to be retrieved by other kernel components later!!!

    ; Save Active Status Register
    in r18, SREG
    sts A_SREG, r18

    ; ***********************************************************
    ; switch to kernel stack:
    lds r19, KSP_HIGH
    lds r20, KSP_LOW
    out SPH, r19
    out SPL, r20

    ; reload the timer   (timer counter 0)
    ldi r16,INITIAL_TIMER0_COUNT
    out TCNT0,r16

    ; goto timer handler
    call K_timer_handler

    ; save the kernel stack pointer
    in r19, SPH
    in r20, SPL
    sts KSP_HIGH, r19
    sts KSP_LOW,  r20
    ; ***********************************************************


    ; Load new active stack pointer
    lds r19, A_SPL
    lds r20, A_SPH
    out SPL, r19
    out SPH, r20

    ; check if there is a return value to give the new process
    lds r16, RETCODEI
    ldi r17, 0x00 ; RETCODEI=0  ==> No return value
    cp r16, r17
    breq TIMERH_NO_RETVAL

    ; restore all registers and save return value:

    ; Load new active status register
    lds r19, A_SREG
    out SREG, r19

    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r18
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    pop r0

    ; save return value - r24:low, r25:high
    lds r24, RETURN_L  ;low
    lds r25, RETURN_H  ;high
    rjmp TIMERH_EXIT

TIMERH_NO_RETVAL:
    ; restore all registers

    ; Load new active status register
    lds r19, A_SREG
    out SREG, r19

    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r18
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    pop r0

TIMERH_EXIT:

    ; jump to new active process
    reti



;********************************************
; Unrecoverable Kernel Error
;********************************************
K_panic:
    ldi r16, 0xff
    out PORTA, r16
    rjmp K_panic
