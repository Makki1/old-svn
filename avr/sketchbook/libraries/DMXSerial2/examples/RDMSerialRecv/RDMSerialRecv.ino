// - - - - -
// DmxSerial2 - A hardware supported interface to DMX and RDM.
// RDMSerialRecv.ino: Sample RDM application.
//
// Copyright (c) 2011-2013 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// 
// This Arduino project is a sample application for the DMXSerial2 library that shows
// how a 3 channel receiving RDM client can be implemented.
// The 3 channels are used for PWM Output:
// address (startAddress) + 0 (red) -> PWM Port 9
// address (startAddress) + 1 (green) -> PWM Port 6
// address (startAddress) + 2 (blue) -> PWM Port 5
//
// Ths sample shows how Device specific RDM Commands are handled in the processCommand function.
// The following RDM commands are implemented here:
// E120_LAMP_HOURS
// E120_DEVICE_HOURS
//
// More documentation and samples are available at http://www.mathertel.de/Arduino
// 06.12.2012 created from DMXSerialRecv sample.
// 09.12.2012 added first RDM response.
// 22.01.2013 first published version to support RDM
// 03.03.2013 Using DMXSerial2 as a library
// - - - - -

#include <EEPROM.h>
#include <DMXSerial2.h>

// Constants for demo program

const int RedPin =    9;  // PWM output pin for Red Light.
const int GreenPin =  6;  // PWM output pin for Green Light.
const int BluePin =   5;  // PWM output pin for Blue Light.

// color: #203050 * 2
#define RedDefaultLevel   0x20 * 2
#define GreenDefaultLevel 0x30 * 2
#define BlueDefaultLevel  0x50 * 2

// define the RGB output color 
void rgb(byte r, byte g, byte b)
{
  analogWrite(RedPin,   r);
  analogWrite(GreenPin, g);
  analogWrite(BluePin,  b);
} // rgb()


void setup () {
  // initialize the Serial interface to be used as an RDM Device Node.
  DMXSerial2.initRDM(RDMDeviceMode,
    3, // footprint
    "www.mathertel.de",   // manufacturerLabel
    "Arduino RDM Device", // deviceModel
    processCommand // RDMCallbackFunction
  );
  
  uint16_t start = DMXSerial2.getStartAddress();
  // set default values to dark red
  // this color will be shown when no signal is present for the first 5 seconds.
  DMXSerial2.write(start + 0, 30);
  DMXSerial2.write(start + 1,  0);
  DMXSerial2.write(start + 2,  0);
  
  // enable pwm outputs
  pinMode(RedPin,   OUTPUT); // sets the digital pin as output
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin,  OUTPUT);
} // setup()


void loop() {
  // Calculate how long no data backet was received
  unsigned long lastPacket = DMXSerial2.noDataSince();

  if (DMXSerial2.isIdentifyMode()) {
    // RDM Command for Indentification was sent.
    // Blink the device.
    unsigned long now = millis();
    if (now % 1000 < 500) {
      rgb(200, 200, 200);
    } else {
      rgb(0, 0, 0);
    } // if
    
  } else if (lastPacket < 30000) {
    // read recent DMX values and set pwm levels 
    analogWrite(RedPin,   DMXSerial2.readRelative(0));
    analogWrite(GreenPin, DMXSerial2.readRelative(1));
    analogWrite(BluePin,  DMXSerial2.readRelative(2));

  } else {
    // Show default color, when no data was received since 30 seconds or more.
    analogWrite(RedPin,   RedDefaultLevel);
    analogWrite(GreenPin, GreenDefaultLevel);
    analogWrite(BluePin,  BlueDefaultLevel);
  } // if
  
  // check for unhandled RDM commands
  DMXSerial2.tick();
} // loop()


// This function was registered to the DMXSerial2 library in the initRDM call.
// Here device specific RDM Commands are implemented.
boolean processCommand(struct RDMDATA *rdm)
{
  byte CmdClass       = rdm->CmdClass;     // command class
  uint16_t Parameter  = rdm->Parameter;	   // parameter ID
  boolean handled = false;

// This is a sample of how to return some device specific data
  if ((CmdClass == E120_GET_COMMAND) && (Parameter == SWAPINT(E120_DEVICE_HOURS))) { // 0x0400
    rdm->DataLength = 4;
    rdm->Data[0] = 0;
    rdm->Data[1] = 0;
    rdm->Data[2] = 2;
    rdm->Data[3] = 0;
    handled = true;

  } else if ((CmdClass == E120_GET_COMMAND) && (Parameter == SWAPINT(E120_LAMP_HOURS))) { // 0x0400
    rdm->DataLength = 4;
    rdm->Data[0] = 0;
    rdm->Data[1] = 0;
    rdm->Data[2] = 0;
    rdm->Data[3] = 1;
    handled = true;
  } // if
  
  return(handled);
} // _processRDMMessage


// End.

