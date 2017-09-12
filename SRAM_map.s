;****************************************************************
;   AVROS: ATmega128 Operating System
;   (c) gamykla
;
;  SYSTEM MEMORY MAP (ASM version)
;
;  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/SRAM_map.s,v $
;  $Author: gamykla $
;  $Date: 2004/04/17 00:39:44 $
;  $Revision: 1.1 $
;
;****************************************************************



;/**********************************************************************
;  WARNING: The following memory organization layout is specific
;           to ATMEL ATmega128 architecture. This section will need to
;           be re-defined if re-compiling for a new architecture.
;***********************************************************************/

; first byte in SRAM
.equ RAMSTART, 0x0100

; Static Kernel Space (512 bytes)
.equ KERNEL_STATIC_DATA_START,          RAMSTART
.equ KERNEL_STATIC_DATA_END ,           0x04e8

; dynamic heap (1024 bytes)
.equ HEAP_START              ,          0x04e9
.equ HEAP_END                ,          0x08d1

; user stack area (2346 bytes)
.equ USER_STACKSPACE_START    ,         0x08d2
.equ USER_STACKSPACE_END      ,         0x1036

; Kernel Stack space (210 bytes)
.equ KERNEL_STACKSPACE_START   ,        0x1037
.equ KERNEL_STACKSPACE_END     ,        RAMEND


; static kernel space:
; ---------------------------

; storage area for kernel stack pointer
.equ KSP_HIGH         ,        0x0100
.equ KSP_LOW        ,          0x0101

; index of active process
.equ ACTIVE         ,          0x0102

; syscal parameters:
.equ OPCODE,                   0x0103

.equ P1_L   ,                  0x0104
.equ P1_H   ,                  0x0105

.equ P2_L   ,                  0x0106
.equ P2_H   ,                  0x0107

.equ P3_L   ,                  0x0108
.equ P3_H   ,                  0x0109

.equ P4_L   ,                  0x010a
.equ P4_H   ,                  0x010b

.equ P5_L   ,                  0x010c
.equ P5_H   ,                  0x010d

; process state data

.equ A_SPL  ,                  0x010e
.equ A_SPH  ,                  0x010f

; return code indicator
.equ RETCODEI  ,               0x0110     ; set to 0x01 if return code present else 0x00
;.equ A_PCH  ,                 0x0111

.equ A_SREG ,                  0x0112

.equ RETURN_L,                 0x0113
.equ RETURN_H,                 0x0114

; temporary storage
.equ TMP_1,                     0x0115
.equ TMP_2,                     0x0116
.equ TMP_3,                     0x0117
.equ TMP_4,                     0x0118
.equ TMP_5,                     0x0119
.equ TMP_6,                     0x011A
.equ TMP_7,                     0x011B
.equ TMP_8,                     0x011C

; process descriptor block
.equ PD_BLOCK_START,           0x011D




