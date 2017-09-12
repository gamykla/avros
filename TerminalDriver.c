/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File              : TerminalDriver.c
* Compiler          : IAR EWAAVR 2.28a
* Version           : 2.1
* Last modified     : 18-07-2003 by LTA
*
* Support mail      : avr@atmel.com
*
* Supported devices : ATmega128 (Modify UART register defines to add support for other devices)
*
* Description       : Terminal emulation interface
*
*
*
****************************************************************************/

#include "TerminalDriver.h"


//***************************
// Misc definitions
// Change for other devices
//***************************

/*** UART registers ***/
#define UARTBAUD        0x09           // UART Baud Rate Register Low
#define UARTSTATUS      0x0B           // UART Status Register
#define UARTCONTROL     0x0A           // UART Control Register
#define UARTDATA        0x0C             // UART Data Register

#define UARTTXEN        3            // Transmitter Enable Bit
#define UARTRXEN        4            // Receiver Enable Bit
#define UARTTXC         6             // Transmit Complete Bit
#define UARTRXC         7            // Receive Complete Bit
#define UARTUDRE        5            // UART Data Register Empty Bit

//#define BAUDRATEFACTOR    1              // Calculated baud rate factor: 115200bps @ 3.69 MHz
#define BAUDRATEFACTOR    11             // Calculated baud rate factor: 19200bps @ 3.69 MHz
//#define BAUDRATEFACTOR    23             // Calculated baud rate factor: 9600bps @ 3.69 MHz



//***************************
// Macros
//***************************

/*** Send escape sequence start ***/
#define SENDESC            \
    Term_Send( 27 );       \
    Term_Send( '[' );



//***************************
// Convert byte to 3 ASCII digits and send
//***************************
void Term_Send_Value_as_Digits( unsigned char value )
{
    unsigned char digit;

    digit = '0';
    while( value >= 100 )                // Still larger than 100 ?
    {
        digit++;                         // Increment first digit
        value -= 100;
    }

    Term_Send( digit );                  // Send first digit

    digit = '0';
    while( value >= 10 )                 // Still larger than 10 ?
    {
        digit++;                         // Increment second digit
        value -= 10;
    }

    Term_Send( digit );                  // Send second digit

    Term_Send( '0' + value );            // Send third digit
}


//***************************
// Initialize UART Terminal
//***************************
void Term_Initialise()
{
    char* ub = (char*)UARTBAUD;
    *ub = BAUDRATEFACTOR;
    //UARTBAUD = BAUDRATEFACTOR;                    // Set baud rate and enable UART

    char* uc = (char*)UARTCONTROL;
    *uc = (1<<UARTTXEN) | (1<<UARTRXEN);
    //UARTCONTROL = (1<<UARTTXEN) | (1<<UARTRXEN);

    Term_Set_Display_Attribute_Mode( MODE_NONE ); // Disable all previous modes
    Term_Erase_Screen();                          // Clear screen
    Term_Set_Cursor_Position( 1, 1 );             // Move to top-left corner
}


//****************************
// Transmit one byte
//****************************
void Term_Send( unsigned char data )
{
    while( !(UARTSTATUS & (1<<UARTUDRE)) );       // Wait for data register ready

    char* ud = (char*)UARTDATA;
    *ud = data;
    //UARTDATA = data;                              // Send byte
}


//***************************
// Receive one byte
//***************************
unsigned char Term_Get()
{
    while( !(UARTSTATUS & (1<<UARTRXC)) );        // Wait for receive complete

    return UARTDATA;                              // Return byte
}


//***************************
// Decode incoming ESC sequence
//***************************
unsigned int Term_Get_Sequence()
{
    unsigned char val;
    unsigned int ret;

    val = Term_Get();
    if( val != 27 )                      // Received ESC ?
    {
        ret = val;                       // If not, return char
    }
    else
    {
        val = Term_Get();
        if( val != '[' )                 // Received '[' ?
        {
            ret = val;                   // If not, return char
        }
        else
        {
            val = Term_Get();

            if( val == 'A' ||
                val == 'B' ||
                val == 'C' ||
                val == 'D' )             // Arrow keys ?
            {
                ret = val << 8;          // Put code in upper byte
            }
            else
            {
                ret = val;               // If not, return char
            }
        }
    }
    return ret;
}


//***************************
// Transmit string from Flash
//
// Returns byte count
//***************************
/*
unsigned char Term_Send_FlashStr( unsigned char __flash * str )
{
    unsigned char count = 0;            // Initialize byte counter

    while( *str != 0 )                  // Reached zero byte ?
    {
        Term_Send( *str );
        str++;                          // Increment ptr
        count++;                        // Increment byte counter
    }

    return count;                       // Return byte count
}
*/

//***************************
// Transmit string from SRAM
//
// Returns byte count
//***************************
unsigned char Term_Send_RAMStr( unsigned char * str )
{
    unsigned char count = 0;            // Initialize byte counter

    while( *str != 0 )                  // Reached zero byte ?
    {
        Term_Send( *str );
        str++;                          // Increment ptr
        count++;                        // Increment byte counter
    }

    return count;                       // Return byte count
}


//***************************
// Send 'clear bottom of screen' sequence
//***************************
void Term_Erase_ScreenBottom()
{
    SENDESC                             // Send escape sequence start

    Term_Send( 'J' );
}


//***************************
// Send 'clear top of screen' sequence
//***************************
void Term_Erase_ScreenTop()
{
    SENDESC                             // Send escape sequence start

    Term_Send( '1' );
    Term_Send( 'J' );
}


//***************************
// Send 'clear screen' sequence
//***************************
void Term_Erase_Screen()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( '2' );
    Term_Send( 'J' );
}


//***************************
// Send 'clear end of line' sequence
//***************************
void Term_Erase_to_End_of_Line()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( 'K' );
}


//***************************
// Send 'clear start of line' sequence
//***************************
void Term_Erase_to_Start_of_Line()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( '1' );
    Term_Send( 'K' );
}


//***************************
// Send 'clear line' sequence
//***************************
void Term_Erase_Line()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( '2' );
    Term_Send( 'K' );
}


//***************************
// Set text mode
//***************************
void Term_Set_Display_Attribute_Mode( unsigned char mode )
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( mode );
    Term_Send( 'm' );
}


//***************************
// Set text colour
//***************************
void Term_Set_Display_Colour( unsigned char fg_bg, unsigned char colour )
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( fg_bg );                 // Select foreground/background
    Term_Send( colour );
    Term_Send( 'm' );
}


//***************************
// Set cursor position
//
// Top-left is (1,1)
//***************************
void Term_Set_Cursor_Position( unsigned char row, unsigned char column )
{
    SENDESC                                        // Send escape sequence start
    
    Term_Send_Value_as_Digits( row );              // Convert row byte
    Term_Send( ';' );
    Term_Send_Value_as_Digits( column );           // Convert column byte
    Term_Send( 'H' );
}


//***************************
// Move cursor
//***************************
void Term_Move_Cursor( unsigned char distance, unsigned char direction )
{
    SENDESC                             // Send escape sequence start
    
    Term_Send_Value_as_Digits( distance );         // Convert distance byte

    Term_Send( direction );
}



//***************************
// Save cursor position
//***************************
void Term_Save_Cursor_Position()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( 's' );
}


//***************************
// Restore cursor position
//***************************
void Term_Restore_Cursor_Position()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( 'u' );
}


//***************************
// Enable scroll for entire screen
//***************************
void Term_Set_Scroll_Mode_All()
{
    SENDESC                             // Send escape sequence start
    
    Term_Send( 'r' );
}


//***************************
// Limit screen scrolling to some lines only
//***************************
void Term_Set_Scroll_Mode_Limit( unsigned char start, unsigned char end )
{
    SENDESC                             // Send escape sequence start
    
    Term_Send_Value_as_Digits( start );            // Convert start line byte
    Term_Send( ';' );
    Term_Send_Value_as_Digits( end );              // Convert end line byte
    Term_Send( 'r' );
}


//***************************
// Send 'print screen' command sequence
//***************************
void Term_Print_Screen()
{
    SENDESC                             // Send escape sequence start
   
    Term_Send( 'i' );
}


//***************************
// Draw frame on terminal client
//***************************

/*
void Term_Draw_Frame( unsigned char top, unsigned char left,
                      unsigned char height, unsigned char width,
                      unsigned char doubleFrame )
{

    static unsigned char __flash edges[] = { 0xda, 0xbf, 0xc0, 0xd9, 0xc4, 0xb3,
                                             0xc9, 0xbb, 0xc8, 0xbc, 0xcd, 0xba };

    unsigned char i = 0;
    height++;
    width++;

   
    Term_Set_Cursor_Position( top, left );                // Corners first
    Term_Send( edges[ doubleFrame * 6 + 0 ] );

    Term_Set_Cursor_Position( top, left + width );
    Term_Send( edges[ doubleFrame * 6 + 1 ] );

    Term_Set_Cursor_Position( top + height, left );
    Term_Send( edges[ doubleFrame * 6 + 2 ] );

    Term_Set_Cursor_Position( top + height, left + width );
    Term_Send( edges[ doubleFrame * 6 + 3 ] );

    for( i = left + 1; i < left + width; i++ )           // Top and bottom edges
    {
        Term_Set_Cursor_Position( top, i );
        Term_Send( edges[ doubleFrame * 6 + 4 ] );

        Term_Set_Cursor_Position( top + height, i );
        Term_Send( edges[ doubleFrame * 6 + 4 ] );
    }

    for( i = top + 1; i < top + height; i++ )            // Left and right edges
    {
        Term_Set_Cursor_Position( i, left );
        Term_Send( edges[ doubleFrame * 6 + 5 ] );

        Term_Set_Cursor_Position( i, left + width );
        Term_Send( edges[ doubleFrame * 6 + 5 ] );
    }
}
*/

//***************************
// Draw menu on terminal client
//***************************
/*
unsigned char Term_Draw_Menu( unsigned char __flash * menu, unsigned char selectPos,
                              unsigned char top, unsigned char left, unsigned char doubleFrame )
{
    unsigned char i, width, height;
    unsigned char __flash * ptr = menu;

    width = height = 0;

    while( *ptr != 0 )                          // Scan through menu string
    {
        height++;                               // Keep track of item count

        if( selectPos == height )
        {
            Term_Set_Display_Attribute_Mode( MODE_REVERSED );  // Reverse selected item
        }
        else
        {
            Term_Set_Display_Attribute_Mode( MODE_NONE );
        }
    
        Term_Set_Cursor_Position( top + height, left + 1 );   // Start of item text
        
        i = 0;
        
        while( *ptr != '\n' )                   // Scan through item text
        {
            Term_Send( *ptr );                  // Print item character
            i++;                                // Item character count
            ptr++;
        }
        
        ptr++;                                  // Skip newline character
        
        if( i > width )
            width = i;                          // Always keep max width
    }

    Term_Set_Display_Attribute_Mode( MODE_NONE );
    if( selectPos == 0 )                        // Draw frame only if selectPos == 0
    {
        Term_Draw_Frame( top, left, height, width, doubleFrame );        
    }
    
    Term_Set_Cursor_Position( top + selectPos, left + 1 );    // Postition at start of selected item
    return height;
}


//***************************
// Handle menu
//***************************
unsigned char Term_Handle_Menu( unsigned char __flash * menu, unsigned char selectPos,
                                unsigned char top, unsigned char left, unsigned char doubleFrame )
{
    unsigned char height;
    unsigned int ret;
        

    height = Term_Draw_Menu( menu, 0, top, left, doubleFrame );
    
    while(1)
    {

        Term_Draw_Menu( menu, selectPos, top, left, doubleFrame );
        
        ret = Term_Get_Sequence();             // Decode ESC sequence
        
        if( ret == 13 )                        // Exit on ENTER
            return selectPos;
            
        if( (ret & 0xff) == 0 )                // Arrow key ?
        {
            ret >>= 8;
            
            if( ret == MOVE_UP )
            {
                selectPos--;                   // Move selection up, with wrap
                if( selectPos < 1 )
                    selectPos = height;
            } else if( ret == MOVE_DOWN )
            {
                selectPos++;                   // Move selection down, with wrap
                if( selectPos > height )
                    selectPos = 1;
            }
        }
    }
}

*/
