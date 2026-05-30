; *********************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;   INTERRUPT VECTOR TABLE
;   This bit should always be found at the start of an image.
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/vector_table.s,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
; *********************************************************

; *************************************************
; WARNING: The vector table definition is specific
; to ATmega128 architecture.
; *************************************************

; The linker script places .vectors before any .text, guaranteeing byte 0x0000.
; Without this section attribute the switch-jump-table avr-gcc emits into .text
; would land before our vector table and displace the reset vector.
.section .vectors,"ax",@progbits
.org    0x0000
    jmp K_init     ; System Reset
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_trap     ; int 4 trap handler - Used for system calls
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp tc0_ovf    ; Timer Counter 0 overflow - used for auto context switches.
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic
    jmp K_panic

.section .text  ; switch back so K_init and ISRs go into .text, not .vectors

