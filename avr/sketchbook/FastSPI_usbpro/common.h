/*
  Common defines
  
*/

#ifndef COMMON_H
#define COMMON_H


/* USBpro-states */
enum {
      U_PRE_SOM = 0,
      U_GOT_SOM = 1,
      U_GOT_LABEL = 2,
      U_GOT_DATA_LSB = 3,
      U_IN_DATA = 4,
      U_WAITING_FOR_EOM = 5,
};

/* USBpro message-labels */
enum {
  PARAMETERS_LABEL = 3,
  DMX_DATA_LABEL = 6,
  SERIAL_NUMBER_LABEL = 10,
  MANUFACTURER_LABEL = 77,
  NAME_LABEL = 78,
  RDM_LABEL = 82, //0x52
};

/* Run-modes */
enum { M_DMX, M_SPECTRUM, M_DEMO2 };

const byte MAX_LABEL_SIZE = 32;

/* class_shit
// device constants
extern char DEVICE_NAME[];
extern byte DEVICE_NAME_SIZE;
extern char MANUFACTURER_NAME[];
extern byte MANUFACTURER_NAME_SIZE;
*/

#endif // COMMON_H
