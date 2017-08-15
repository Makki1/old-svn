/*
  Debugging-macros
*/


#ifndef DEBUG_H
#define DEBUG_H

/* Debugging macros
*/
//#if DEBUG
// LS(L) LogString(newLine) in flash
// LV(H) LogValue(Hex)
#if DEBUG && defined(__AVR_ATmega2560__) // debug-out only on Mega2560 / Serial3
  #define DEBUG_INIT Serial3.begin(115200);
  #define LS(x)    Serial3.print (F(x));
  #define LSL(x)   Serial3.print(millis()); Serial3.print(":"); Serial3.println (F(x));
  #define LV(x)    Serial3.print (x);
  #define LVH(x)   Serial3.print(" 0x"); Serial3.print (x, HEX); Serial3.print(" ");
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

//TODO: SoftSerial for debug-out on 328?
#else
  #define DEBUG_INIT
  #define LS(x)
  #define LSL(x)
  #define LV(x)
  #define LVH(x)
  #define LL
  #define DEBUG_PRINT(x)
#endif // DEBUG


#endif //DEBUG_H
