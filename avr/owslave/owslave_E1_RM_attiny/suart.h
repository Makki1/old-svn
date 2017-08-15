#ifndef _suart_h_
#define _suart_h_

			// size must be in range 2 .. 256
#define STX_SIZE	32
#define	SRX_SIZE	32 //WARNING: 16 causes some losses (onBatt always true?) and ow-errors!, whyever..
#define F_CPU 8000000UL
/* suart */
#define BAUD    9600
#define STXD        SBIT( PORTA, PA6 )  // = OC1A
#define STXD_DDR    SBIT( DDRA,  PA6 )
#define SRXD_PIN    SBIT( PINA,  PA7 )  // = ICP

#define	uputs(x)	uputs_((u8*)(x))	// avoid char warning


void suart_init( void );
void uputchar( u8 c );			// send byte
void uputs_( u8 *s );			// send string from SRAM
u8 kbhit( void );			// check incoming data
u8 ugetchar( void );			// get byte

#endif
