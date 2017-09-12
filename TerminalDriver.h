
/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File              : TerminalDriver.h
* Compiler          : IAR EWAAVR 2.28a
* Version           : 2.1          
* Last modified     : 18-07-2003 by LTA
*
* Support mail      : avr@atmel.com
*
* Description       : Terminal emulation interface header file
*
*
*
****************************************************************************/

#ifndef TERMINALDRIVER_H
#define TERMINALDRIVER_H

//#include <ioavr.h>
//#include <inavr.h>

//#include <avr/ina90.h>
//#include <avr/io.h>


//***************************
// Misc definitions
//***************************

/*** Text modes ***/
#define MODE_NONE         '0'
#define MODE_BOLD         '1'
#define MODE_DIM          '2'
#define MODE_UNDERLINE    '4'
#define MODE_BLINK        '5'
#define MODE_REVERSED     '7'
#define MODE_CONCEALED    '8'


/*** Text colours ***/
#define COL_FOREGROUND    '3'
#define COL_BACKGROUND    '4'

#define COL_BLACK         '0'
#define COL_RED           '1'
#define COL_GREEN         '2'
#define COL_YELLOW        '3'
#define COL_BLUE          '4'
#define COL_MAGENTA       '5'
#define COL_CYAN          '6'
#define COL_WHITE         '7'


/*** Cursor move direction ***/
#define MOVE_UP           'A'
#define MOVE_DOWN         'B'
#define MOVE_RIGHT        'C'
#define MOVE_LEFT         'D'



//***************************
// Function prototypes
//***************************

unsigned char Term_Get();
/*
unsigned char Term_Handle_Menu( unsigned char __flash * menu, unsigned char selectPos,
                                unsigned char top, unsigned char left, unsigned char doubleFrame );

unsigned char Term_Draw_Menu( unsigned char __flash * menu, unsigned char selectPos,
                              unsigned char top, unsigned char left, unsigned char doubleFrame );
*/
unsigned int  Term_Get_Sequence();
//unsigned char Term_Send_FlashStr( unsigned char __flash * str );
unsigned char Term_Send_RAMStr( unsigned char * str );
void Term_Draw_Frame( unsigned char top, unsigned char left,
                      unsigned char height, unsigned char width, unsigned char doubleFrame );
void Term_Erase_Line();
void Term_Erase_Screen();
void Term_Erase_ScreenBottom();
void Term_Erase_ScreenTop();
void Term_Erase_to_End_of_Line();
void Term_Erase_to_Start_of_Line();
void Term_Initialise();
void Term_Move_Cursor( unsigned char distance, unsigned char direction );
void Term_Print_Screen();
void Term_Restore_Cursor_Position();
void Term_Save_Cursor_Position();
void Term_Send( unsigned char data );
void Term_Send_Value_as_Digits( unsigned char value );
void Term_Set_Cursor_Position( unsigned char row, unsigned char column );
void Term_Set_Display_Attribute_Mode( unsigned char mode );
void Term_Set_Display_Colour( unsigned char fg_bg, unsigned char colour );
void Term_Set_Scroll_Mode_All();
void Term_Set_Scroll_Mode_Limit( unsigned char start, unsigned char end );


#endif
