
/****************************************************************
   AVROS: ATmega128 Operating System
   (c) gamykla

  SYSTEM MEMORY MAP 

  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/SRAM_map.h,v $
  $Author: gamykla $
  $Date: 2004/04/17 00:39:44 $
  $Revision: 1.1 $
****************************************************************/

#ifndef _SRAM_map_H_
#define _SRAM_map_H_


/**********************************************************************
  WARNING: The following memory organization layout is specific
           to ATMEL ATmega128 architecture. This section will need to
           be re-defined if re-compiling for a new architecture.
***********************************************************************/

// first byte in SRAM
#define RAMSTART 0x0100
#define RAMEND 0x10ff

// Static Kernel Space (512 bytes)
#define KERNEL_STATIC_DATA_START           RAMSTART
#define KERNEL_STATIC_DATA_END             0x04e8

// User Data areas:
#define HEAP_START                         0x04e9
#define HEAP_END                           0x08d1

// user stack area:
#define USER_STACKSPACE_START              0x08d2
#define USER_STACKSPACE_END                0x1036

// Kernel Stack space:
#define KERNEL_STACKSPACE_START            0x1037
#define KERNEL_STACKSPACE_END              RAMEND

// SPH=0x10      -> origin of kernel stack area.
// SPL=0xff

// static kernel space:
// ---------------------------

// storage area for kernel stack pointer
#define KSP_HIGH                 0x0100
#define KSP_LOW                  0x0101
#define ACTIVE                   0x0102    // index of active

// syscal parameters:
#define OPCODE                    0x0103   // system call opcode
#define  P1_L                     0x0104   // paramater 1
#define  P1_H                     0x0105
#define  P2_L                     0x0106   // paramater 2
#define  P2_H                     0x0107
#define  P3_L                     0x0108   // paramater 3
#define  P3_H                     0x0109
#define  P4_L                     0x010a   // paramater 4
#define  P4_H                     0x010b
#define  P5_L                     0x010c   // paramater 5
#define  P5_H                     0x010d

#define  A_SPL                    0x010e   // Active Stack pointer
#define  A_SPH                    0x010f

// return code indicator
#define  RETCODEI                 0x0110

//#define  A_PCH                    0x0111
#define  A_SREG                   0x0112   // Active status register
#define  RETURN_L                 0x0113   // Return Code
#define  RETURN_H                 0x0114

// temporary storage area
#define TMP_1                     0x0115
#define TMP_2                     0x0116
#define TMP_3                     0x0117
#define TMP_4                     0x0118
#define TMP_5                     0x0119
#define TMP_6                     0x011A
#define TMP_7                     0x011B
#define TMP_8                     0x011C

// Process Descriptor Array:
#define PD_BLOCK_START            0x011D

#endif

