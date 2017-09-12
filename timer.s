;****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  HARWARE TIMING DEFINITIONS
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/timer.s,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
; *********************************************************
; NOTE: THIS FILE MUST BE IN SYNCHRONIZATION WITH timer.h
; *********************************************************
;
;
; WARNING: these timer pre-scalar settings are hardware specific to
; ATmega128 architecture. Adjustments and re-calculation may be
; required for different processors.
;

;****************************************************************



; We'll operate at a clock speed of 1 Megahertz (F_Ck=1 Mhz),
; so we need an init. timer count of 0d157 to generate
; 10 timer interrupts per second (Hz = 10).
; i.e) A timer interrupt will occur every 100 MS.


; Hz = (F_Ck)/[(1024)*(255-INITAL_TIMER0_COUNT)]
.equ INITIAL_TIMER0_COUNT , 0x9D ; 157
.equ MSEC, 0x64 ; 100


;TCCR0 pre-scalar options for the ATmega128
.equ DISABLE_TCNT0   , 0b000
.equ CPRESCALAR_1    , 0b001
.equ CPRESCALAR_8    , 0b010
.equ CPRESCALAR_32   , 0b011
.equ CPRESCALAR_64   , 0b100
.equ CPRESCALAR_128  , 0b101
.equ CPRESCALAR_256  , 0b110


; a timer prescaler at 1024 indicates that we will increment the
; counter register every 1024 clock cycles.
; Thus, with INITIAL_TIMER0_COUNT = 157, the CPU will go through
; 1024*(255-157) ~ 100,000 clock cycles (or aprox 100,000 instructions)
; before TIMER0 overflows triggering an interrupt. (Note that timer/counter0
; is an 8 bit counter.)
.equ CPRESCALAR_1024 , 0b111

