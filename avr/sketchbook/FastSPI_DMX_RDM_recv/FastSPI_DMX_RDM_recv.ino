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
// FastSPI_LED: 
#include <FastSPI_LED.h>

// DMXSerial2: Constants for demo program
const int RedPin =    3;  // PWM output pin for Red Light.
const int GreenPin =  6;  // PWM output pin for Green Light.
const int BluePin =   9;  // PWM output pin for Blue Light.
const int WhitePin =  10;  // PWM output pin for Blue Light.

// DMXSerial2: color: #203050 * 2
#define RedDefaultLevel   0x20 * 2
#define GreenDefaultLevel 0x30 * 2
#define BlueDefaultLevel  0x50 * 2

// DMXSerial2: define the RGB output color 
void rgb(byte r, byte g, byte b)
{
  analogWrite(RedPin,   r);
  analogWrite(GreenPin, g);
  analogWrite(BluePin,  b);
} // rgb()

// FastSPI_LED: wiring of LEDs/colors on chip
//BLIPTRONICS: 
struct CRGB { unsigned char g; unsigned char b; unsigned char r; };
//DycoLED: struct CRGB { unsigned char r; unsigned char b; unsigned char g; };
struct CRGB *leds;
#define NUM_LEDS 50
#define DMX_CHANNELS NUM_LEDS*3+9
//Channel-assignment: (ex20,60,160)
//Chan 1-X*3 = Pixels RGB (1-60, 1-180, 1-480)
//4x RGBW (61,181,481+)
//3x RGB All Pixels (65,185,485)
//Demo-program (68,188,488)
//Demo speed (multiplier 10ms) 69,189,489
//Numpixels/X/Y??

//Spectrum-analyzer:
int spectrumReset=5;
int spectrumStrobe=4;
int spectrumAnalog=0;  //0 for left channel, 1 for right.
// Spectrum analyzer read values will be kept here.
int Spectrum[7];


void setup () {
  // initialize the Serial interface to be used as an RDM Device Node.
  DMXSerial2.initRDM(RDMDeviceMode,
    DMX_CHANNELS, // footprint
    "www.wiregate.de",   // manufacturerLabel
    "Pixelv2 RDM Device", // deviceModel
    processCommand // RDMCallbackFunction
  );
  
  uint16_t start = DMXSerial2.getStartAddress();
  // set default values to dark red
  // this color will be shown when no signal is present for the first 5 seconds.
  //DMXSerial2.write(start + 0, 30);
  //DMXSerial2.write(start + 1,  0);
  //DMXSerial2.write(start + 2,  0);
  
  // enable pwm outputs
  pinMode(RedPin,   OUTPUT); // sets the digital pin as output
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin,  OUTPUT);
  // FastSPI_LED: 
  FastSPI_LED.setLeds(NUM_LEDS);
  // LPD6803, 5bit/color for Bliptronics, DycoLED, MagiarLED etc.
  FastSPI_LED.setChipset(CFastSPI_LED::SPI_LPD6803);
  FastSPI_LED.setDataRate(4); //6=FOSC/64 = 125kHz, 4= FOSC/32 = 250kHz?
  //FastSPI_LED.setPin(PIN); //only needed for UCS1803 or TM1809 
  FastSPI_LED.init();
  FastSPI_LED.start();
  leds = (struct CRGB*)FastSPI_LED.getRGBData();
  // clear all
  FastSPI_LED.show();

  //Spectrum analyzer:
  //Setup pins to drive the spectrum analyzer. 
  pinMode(spectrumReset, OUTPUT);
  pinMode(spectrumStrobe, OUTPUT);
  digitalWrite(spectrumStrobe,LOW);
    delay(1);
  digitalWrite(spectrumReset,HIGH);
    delay(1);
  digitalWrite(spectrumStrobe,HIGH);
    delay(1);
  digitalWrite(spectrumStrobe,LOW);
    delay(1);
  digitalWrite(spectrumReset,LOW);
    delay(5);
  // Reading the analyzer now will read the lowest frequency.
} 

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
  } else if (lastPacket < 30000 && (DMXSerial2.read(NUM_LEDS*3+8)==0 || DMXSerial2.read(NUM_LEDS*3+8)==255)) {
    //set single pixels by channel
    //FIXME: !! use setRGBData en block !!
    for (int chan=0;chan<DMX_CHANNELS;chan++)
      setChannel(chan,DMXSerial2.read(chan+1)); //readRelative(chan+1));
  } else if (lastPacket < 30000 && DMXSerial2.read(NUM_LEDS*3+8)==1) {
    //set all pixels color
    for (int led=0;led<NUM_LEDS;led++) {
      leds[led].r = DMXSerial2.read(NUM_LEDS*3+5);
      leds[led].g = DMXSerial2.read(NUM_LEDS*3+6);
      leds[led].b = DMXSerial2.read(NUM_LEDS*3+7);
    }
  } else if (lastPacket < 30000 && DMXSerial2.read(NUM_LEDS*3+8)>1 && DMXSerial2.read(NUM_LEDS*3+8)<10) {
    runDemo(DMXSerial2.read(NUM_LEDS*3+8));
  } else if (lastPacket < 30000 && DMXSerial2.read(NUM_LEDS*3+8)==10) {
    while (DMXSerial2.read(NUM_LEDS*3+8)==10) {
      showSpectrum();
      delay(15);
    }
  } else {
    // Show default color, when no data was received since 30 seconds or more.
    analogWrite(RedPin,   RedDefaultLevel);
    analogWrite(GreenPin, GreenDefaultLevel);
    analogWrite(BluePin,  BlueDefaultLevel);
  } 
  FastSPI_LED.show();
  //this is STRANGE! a delay below 50ms for 20 NUM_LEDS gives massive flickering, maybe .show() should be synced to End-of-DMX or so..
  delay(125); //125 for 50 LEDS
  
  // check for unhandled RDM commands
  DMXSerial2.tick();
} // loop()


void setChannel(int chan, byte val) {
  byte color = chan % 3;
  //FIXME: !! use setRGBData en block !!
  if (chan < NUM_LEDS*3) //only set single channels if in DMX-mode
    switch (color) {
        case 0: leds[int(chan/3)].r = val; break;
        case 1: leds[int(chan/3)].g = val; break;
        case 2: leds[int(chan/3)].b = val; break;
    }
  else if (chan < (NUM_LEDS*3+4)) {
      analogWrite(RedPin,   DMXSerial2.read(NUM_LEDS*3+1));
      analogWrite(GreenPin, DMXSerial2.read(NUM_LEDS*3+2));
      analogWrite(BluePin,  DMXSerial2.read(NUM_LEDS*3+3));
      //analogWrite(WhitePin,  DMXSerial2.read(NUM_LEDS*3+4));
  }
}


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


void runDemo(uint8_t demonr) {
  //TODO: add more demos
  int basedelay;
  if (DMXSerial2.read(NUM_LEDS*3+9) > 0) 
    basedelay = DMXSerial2.read(NUM_LEDS*3+9);


  switch (demonr) {
    case 2:
    {
      // one at a time
      for(int j = 0; j < 4; j++) { 
        for(int i = 0 ; i < NUM_LEDS; i++ ) {
          memset(leds, 0, NUM_LEDS * 3);
          switch(j) { 
            case 0: leds[i].r = 255; break;
            case 1: leds[i].g = 255; break;
            case 2: leds[i].b = 255; break;
            case 3: 
              leds[i].r = 255; 
              leds[i].g = 255; 
              leds[i].b = 255; 
              break;
          }
          FastSPI_LED.show();
          delay(100); //20
          //bail out if demo changed
          if (DMXSerial2.read(NUM_LEDS*3+8) != demonr)
            break;
        }
      }
    }
    break;
    case 3:
    {
      // growing/receeding bars
      for(int j = 0; j < 3; j++) { 
        memset(leds, 0, NUM_LEDS * 3);
        for(int i = 0 ; i < NUM_LEDS; i++ ) {
          switch(j) { 
            case 0: leds[i].r = 255; break;
            case 1: leds[i].g = 255; break;
            case 2: leds[i].b = 255; break;
          }
          FastSPI_LED.show();
          delay(50); //10
          //bail out if demo changed
          if (DMXSerial2.read(NUM_LEDS*3+8) != demonr)
            break;
        }
        delay(100);
        for(int i = NUM_LEDS-1 ; i >= 0; i-- ) {
          switch(j) { 
            case 0: leds[i].r = 0; break;
            case 1: leds[i].g = 0; break;
            case 2: leds[i].b = 0; break;
          }
          FastSPI_LED.show();
          delay(50); //10
          //bail out if demo changed
          if (DMXSerial2.read(NUM_LEDS*3+8) != demonr)
            break;
        }
      }
    }
    break;
    case 4:
    {
      // Fade in/fade out
      for(int j = 0; j < 4; j++ ) {
        memset(leds, 0, NUM_LEDS * 3);
        for(int k = 0; k < 256; k++) { 
          for(int i = 0; i < NUM_LEDS; i++ ) {
            switch(j) {
              case 0: leds[i].r = k; break;
              case 1: leds[i].g = k; break;
              case 2: leds[i].b = k; break;
              case 3: 
                leds[i].r = k; 
                leds[i].g = k; 
                leds[i].b = k; 
                break;
            }
          }
          FastSPI_LED.show();
          delay(3); //3
        }
        for(int k = 255; k >= 0; k--) { 
          for(int i = 0; i < NUM_LEDS; i++ ) {
            switch(j) { 
              case 0: leds[i].r = k; break;
              case 1: leds[i].g = k; break;
              case 2: leds[i].b = k; break;
              case 3: 
                leds[i].r = k; 
                leds[i].g = k; 
                leds[i].b = k; 
                break;
            }
          }
          FastSPI_LED.show();
          delay(3); //3
        }
      }
      //bail out if demo changed
      if (DMXSerial2.read(NUM_LEDS*3+8) != demonr)
        break;
    }
    break;
    case 5:
    {
      //rainbow red-yellow-green-turkey-blue
      for (int i=0;i<NUM_LEDS-4; i++) {
        memset(leds, 0, NUM_LEDS * 3);
        leds[i].r = 0;
        leds[i].g = 0;
        leds[i].b = 255;
        leds[i+1].r = 0;
        leds[i+1].g = 254;
        leds[i+1].b = 255;
        leds[i+2].r = 0;
        leds[i+2].g = 255;
        leds[i+2].b = 0;
        leds[i+3].r = 255;
        leds[i+3].g = 255;
        leds[i+3].b = 0;
        leds[i+4].r = 255;
        leds[i+4].g = 0;
        leds[i+4].b = 0;
        FastSPI_LED.show();
        delay(100);
      }
    }
  }

} // end demo

// Read 7 band equalizer.
void readSpectrum()
{
  // Band 0 = Lowest Frequencies.
  byte Band;
  for(Band=0;Band <7; Band++)
  {
    Spectrum[Band] = (analogRead(spectrumAnalog) + analogRead(spectrumAnalog) ) >>1; //Read twice and take the average by dividing by 2
    //FIXME: spectrumAnalog=0=left only, maybe use L+R?
    digitalWrite(spectrumStrobe,HIGH);
    digitalWrite(spectrumStrobe,LOW);     
  }
}


void showSpectrum() {
  //Not I don;t use any floating point numbers - all integers to keep it zippy. 
  readSpectrum();
  byte Band, BarSize, MaxLevel;
  static unsigned int  Divisor = 80, ChangeTimer=0; //, ReminderDivisor,
  unsigned int works, Remainder;
  
  MaxLevel = 0; 
        
  for(Band=0;Band<7;Band++) {
    //If value is 0, we don;t show anything on graph
    works = Spectrum[Band]/Divisor;	//Bands are read in as 10 bit values. Scale them down to be 0 - 5
    if(works > MaxLevel)  //Check if this value is the largest so far.
      MaxLevel = works;                       
    for(BarSize=1;BarSize <=5; BarSize++) {
    //setLEDFast = LED,r,g,b
      if(works > BarSize) { //LP.setLEDFast(	GridTranslate(Band,BarSize-1),BarSize*6,31-(BarSize*5),0);
          leds[GridTranslate(Band,BarSize-1)].r = BarSize*6;
          leds[GridTranslate(Band,BarSize-1)].g = 31-(BarSize*5);
          leds[GridTranslate(Band,BarSize-1)].b = 0;
      } else if (works == BarSize) { //LP.setLEDFast(	LP.Translate(Band,BarSize-1),BarSize*6,31-(BarSize*5),0); //Was remainder
          leds[GridTranslate(Band,BarSize-1)].r = BarSize*6;
          leds[GridTranslate(Band,BarSize-1)].g = 31-(BarSize*5);
          leds[GridTranslate(Band,BarSize-1)].b = 0;
      } else {
          leds[GridTranslate(Band,BarSize-1)].r = 5;
          leds[GridTranslate(Band,BarSize-1)].g = 0;
          leds[GridTranslate(Band,BarSize-1)].b = 5;
      }
    }
  }
  FastSPI_LED.show();

 // Adjust the Divisor if levels are too high/low.
 // If  below 4 happens 20 times, then very slowly turn up.
  if (MaxLevel >= 5) {
      Divisor=Divisor+1;
      ChangeTimer=0;
  } else if(MaxLevel < 4) {
      if(Divisor > 65)
        if(ChangeTimer++ > 20) {
          Divisor--;
          ChangeTimer=0;
        }
  } else {
      ChangeTimer=0; 
  }
}

//Translate x and y to a LED index number in an array.
//Assume LEDS are layed out in a zig zag manner eg for a 3x3:
//0 5 6
//1 4 7
//2 3 8
unsigned int GridTranslate(byte x, byte y) {
  uint8_t gridHeight = 5;
  if(x%2) {
    return(((x+1) * gridHeight)- 1 - y);
  } else {
    return((x * gridHeight) + y);
  }
}

