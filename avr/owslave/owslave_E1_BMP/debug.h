#ifndef DEBUG_H
#define DEBUG_H

#if DEBUG
/* delay.h fucks up the serial-timing somehow if F_CPU is not defined!
 * best: avoid using delay at all!
 * second-best: define it here if undef but this might result in diffenrences when debug is disabled!
 */

#ifndef F_CPU
# warning "F_CPU was not defined!! defining it now in debug.h but you should take care before!"
#define F_CPU 8000000UL //very important! define before delay.h as delay.h fucks up the serial-timing otherwiese somehow
#endif

#ifdef _UTIL_DELAY_H_
# error "do not use delay.h besides for debugging!"
#endif
#include <util/delay.h>

#define UART_BAUD_RATE 38400
#define DEBUG_INIT uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
#define LSL(x)    uart_puts_P(x);
#define LS(x)    uart_puts_P(x);
#define LL        uart_puts_P("\r\n");
#define LV_SETUP  char buffer[12];
#define LV(x)     uart_puts(ultoa( x, buffer, 10));
#define LVH(x)    uart_puts(ultoa( x, buffer, 16));

#define DLY(x)  _delay_ms(x);

/*
char buffer[7];
itoa( cbuf, buffer, 16);   // convert interger into string (decimal format)
//uart_puts(buffer);        // and transmit string to UART_BAUD_RATE
*/

  /*  #define LS(x)    Serial3.print (F(x));
  #define LSL(x)   Serial3.print(millis()); Serial3.print(":"); Serial3.println (F(x));
  #define LV(x)    Serial3.print (x);
  #define LVH(x)   Serial3.print(" 0x"); Serial3.print (x, HEX); Serial3.print(" ");
  #define LVB(x)   Serial3.print(" B"); Serial3.print (x, BIN); Serial3.print(" ");
  #define LVD(x)   Serial3.print(" D"); Serial3.print (x, DEC); Serial3.print(" ");
  #define LL       Serial3.println();
  #define DEBUG_PRINT(x)        \
    Serial3.print(millis());     \
    Serial3.print(": ");         \
    Serial3.print(__PRETTY_FUNCTION__); \
    Serial3.print(' ');          \
    Serial3.print(__FILE__);     \
    Serial3.print(':');          \
    Serial3.print(__LINE__);     \
    Serial3.print(' ');          \
    Serial3.println(x);
// freeRam()
*/

#else

/* alternative empty macros for the above */
  #define DEBUG_INIT
  #define LS(x)
  #define LSL(x)
  #define LV(x)
  #define LVH(x)
  #define LVB(x)
  #define LVD(x)
  #define LL
  #define DEBUG_PRINT(x)
  #define DLY(x)
  #define LV_SETUP

#endif //DEBUG

#endif //DEBUG_H

