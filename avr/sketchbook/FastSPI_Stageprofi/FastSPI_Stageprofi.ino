/* Pixelcontroller - Stageprofi 2013-04-28
   Implements DMX4All Stageprofi protocol
   *** Proof of concept only ! ***
   Though, it works if it's being talked to slowly and read the output-pipe
   See enclosed README for concept, licenses, (C), Credits etc.
    
   (C) 2013 Michael Markstaller / Elaborated Networks GmbH
*/

// FastSPI v20121014
#include <FastSPI_LED.h>

#define DEBUG
// #undef DEBUG 

#if defined(__AVR_ATmega2560__) // debug-out only on Mega2560 / Serial3
#ifdef DEBUG
// LS(L) LogString(newLine) in flash
// LV(H) LogValue(Hex)
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
#else
  #define DEBUG_INIT
  #define LS(x)
  #define LSL(x)
  #define LV(x)
  #define LVH(x)
  #define LL
  #define DEBUG_PRINT(x)
#endif // DEBUG
#endif // only on Mega2560

// free RAM approx 1202 @170, 1707 @20

// Sometimes chipsets wire in a backwards sort of way
//struct CRGB { unsigned char b; unsigned char r; unsigned char g; };
//struct CRGB { unsigned char r; unsigned char g; unsigned char b; };
struct CRGB { unsigned char g; unsigned char b; unsigned char r; };
struct CRGB *leds;

//Init always max. num_leds for smotth demo/grid is set by DMX
#define MAX_LEDS 168

enum { M_DMX, M_SPECTRUM, M_DEMO2 };
byte gModeb[] = { 0, 0, 0, 0, 2, 20, 5, 4 }; //Defaults: DMX-mode/Demo (5), numleds, gridX, gridY

/* Control-states: 00, 0xFF, 0xE2, 0xE3 (E0/E1??)
  and ASCII; aaa = channel (000...511) bbb = value (000...255)
  C(check), C? -> G
  I(nfo), I -> ASCII-version
  C(hannel set), CaaaLbbb -> G
  C(hannel get), Caaa? -> bbbG
*/
enum { S_IDLE, S_BULKWRITE, S_COUT_LOW, S_COUT_HIGH, S_BULKREAD, 
       S_CA, S_CACHECK, S_CASET, S_CAGET }; 
byte state = S_IDLE;

#define DMX_RCHAR 'G'
#define MAX_MILLIS_TO_WAIT 1000  //serial timeout

/*
#define FS(x) (__FlashStringHelper*)(x)
const char MyText[]  PROGMEM  = { "My flash based text" };
Serial.println(FS(MyText));
*/

void setup()
{
  DEBUG_INIT
  LL LL LSL("Debug-Port enabled")
  DEBUG_PRINT("oder so")
  pinMode(3, OUTPUT);   // set the PWM-pins as output
  pinMode(6, OUTPUT);   // set the PWM-pins as output
  pinMode(9, OUTPUT);   // set the PWM-pins as output
  //FIXME: unused pins low, output-pins?
  //TODO EEprom save/restore?
//  attachInterrupt(0, buttonPress, LOW); //PIN2 / INT0
  
  FastSPI_LED.setLeds(MAX_LEDS);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_SM16716);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_TM1809);
  FastSPI_LED.setChipset(CFastSPI_LED::SPI_LPD6803);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_HL1606);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_595);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_WS2801);

  //FastSPI_LED.setPin(PIN); //only needed for UCS1803 or TM1809 
  
  FastSPI_LED.init();
  FastSPI_LED.start();
  leds = (struct CRGB*)FastSPI_LED.getRGBData();
  // clear all
  FastSPI_LED.show();
  
  // initialize the serial communication:
  Serial.begin(38400);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
}

void loop() {
  processSerial();
  //TODO: PIN-out?
  //TODO EEpromsave-button?
  //FastSPI_LED.show(); // not required here..

  //FIXME: switch different demos, spectrum-analyzer
  //box, line
  if (gModeb[4] == 2) {
    runDemo();
  } else if (gModeb[4] == 0) {
    analogWrite(3, gModeb[0]);
    analogWrite(6, gModeb[1]);
    analogWrite(9, gModeb[2]);
  }
  //FIXME: unknown, just to test..
  //Serial.flush();
}

void setChannel(int chan, byte val) {

  byte color = chan % 3;
  if (chan < 505 && gModeb[4] == 0) //only set single channels if in DMX-mode
    switch (color) {
        case 0: leds[int(chan/3)].r = val; break;
        case 1: leds[int(chan/3)].g = val; break;
        case 2: leds[int(chan/3)].b = val; break;
    }
    else if (chan < 512)
        gModeb[chan-504] = val;
}

void buttonPress () {
}


void processSerial() {
  //FIXME: send this to a func to be able to irq demo etc.
  //DEBUG: receiving full 259 bytes frame (255 values) should take smthg. like
  // 259 * 10(bits per char) = 2590 
  // 38400 / 10 = 3840 chars/sec = reading a frame blocks for 1/14s = 71ms
  // this will make demo "hopping" if DMX-data is steady recv'd
  // input-buffer (default) = 64(?) bytes so we should not sleep longer than 16ms !
  // Or make it bigger in hardware/arduino/cores/arduino/HardwareSerial.cpp ?! 
  
  // recv and set per channel seems to be too slow! so we save the bytes
  //static byte message[600]; //actually its max. 255 for stageprofi and 600 for usbpro!
  //static uint8_t datalen = 0;
  //TODO BREAK:  Rewrite this stuff like UsbProReceiver.cpp !
  
  while (Serial.available() > 0) {
    char inB;
    LSL("Serial BEGIN")
    /*
    FIXME: maybe run demo when theres > 10sec no serial signal?
    if (gModeb[4] == 2) {
      Serial.println("DEBUG: demo off:");
      Serial.println(gModeb[4]);
      Serial.println(gModeb[5]);
      memset(leds, 0, gModeb[5] * 3); // all off
      gModeb[4] = 0; // no demo after first command
    }
    */
    //FIXME: we never get out of demo again with OLA
    if (state == S_IDLE) {
      inB = Serial.read();
      LS(" inB") LVH(inB)    
      switch (inB) {
        case 0xFF: // up to 255 DMX bytes follow
          state = S_BULKWRITE; LSL("S_I->S_B")
          break;
        case 0xE0: // Single LOW channel with response G (undocumented?)
        case 0xE2: // Single LOW channel -fast mode
          state = S_COUT_LOW;
          break;
        case 0xE1: // Single HIGH channel with response G (undocumented?)
        case 0xE3: // Single HIGH channel -fast mode
          state = S_COUT_HIGH;
          break;
        case 0xFE: // Bulkread
          state = S_BULKREAD;
          break;
        case 'C':  // Set channel - ASCII-mode
          state = S_CA;
          break;
        case 'I':  // Interface info - ASCII-mode
          Serial.print(F("Pixelcontroller Stageprofi-USB clone up "));
          Serial.print(millis()/1000);
          Serial.print(F("s, codebase: "));
          Serial.println(F(__DATE__));
          Serial.print(F("Variant / Pixels:"));
          Serial.print(F("LPD6803 / "));
          Serial.println(gModeb[5]);
          Serial.print(F("RAM free:"));
          Serial.println(freeRam()); //1708 right now
          Serial.println(FastSPI_LED.getCycleTarget());
          state = S_IDLE;
          break;
        case 'D':  // Extra: run demo-loop
          if (gModeb[4] == 2)
            gModeb[4] = 0;
          else
            gModeb[4] = 2;
          state = S_IDLE;
          break;
        default:
          //FIXME: just ignore this!
          LS("Ign 0x") LVH(inB)
          LL LS("in inbuffer/dropping") LV(Serial.available()) LL
          //clear inbuffer
          while (Serial.available()) { 
            byte tmp = Serial.read(); 
            LVH(tmp)
          }
          state = S_IDLE;
          break;
      }
    } else {
      unsigned long starttime = millis();
      switch (state) {
        case S_BULKWRITE:
          LSL("S_B->")
          // This is still broken with OLA somehow too fast or smthg locks up..
          // OLA sends: start 0 + FF values, then start FF + FF values, then start FE + Highbyte + 2 values
          while ( (Serial.available()<3) && ((millis() - starttime) < MAX_MILLIS_TO_WAIT) ) {
            LS("W1.")
          }
          if (Serial.available()>=3) {
            //read start channel, highbit, numchannels
            int startchan = Serial.read();
            byte highbyte = Serial.read();
            byte numchannels = Serial.read();
            int val;
            if (highbyte)
              startchan += 256;
            int chan = startchan;
            LSL("bulkwrite start: ") LV(chan) LS(" high/num ") LVH(highbyte) LV(numchannels) LL
            //FIXME: maybe we're too slow or fast here, either read all bytes (max 255) at once into buffer or do a while i<numchannels + timeout
            for (int i = 0; i < numchannels; i++) {
              while ( (Serial.available()<1) && ((millis() - starttime) < MAX_MILLIS_TO_WAIT) ) {
                  LS("W2.")
                  //wait for data - reading too fast otherwise
              }
              chan = startchan + i;
              val = Serial.read();
              if (val == -1)
                continue;
              LV(chan) LVH(val)
              //example ff 3 0 9 0 ff 00 00 ff 00 00 ff 00
              setChannel(chan,val);
            }
            LSL(" end DATA")
            FastSPI_LED.show();
            //no feedback for OLA?
            Serial.write(DMX_RCHAR);
          }
          state = S_IDLE;
          break;
        case S_COUT_LOW:
        case S_COUT_HIGH:
          while ( (Serial.available()<2) && ((millis() - starttime) < MAX_MILLIS_TO_WAIT) ) {
          }
          if (Serial.available()>=2) {
            int chan = Serial.read();
            if (state == S_COUT_HIGH)
              chan += 256;
            byte val = Serial.read();
            setChannel(chan,val);
            FastSPI_LED.show();
            Serial.write(DMX_RCHAR);
          }
          state = S_IDLE;
          break;
        case S_BULKREAD:
          //FIXME: unimplemented
          break;
        case S_CA:
          while ( (Serial.available()<3) && ((millis() - starttime) < MAX_MILLIS_TO_WAIT) ) {
            if (Serial.peek() == '?') {
              inB = Serial.read();
              Serial.write(DMX_RCHAR);
              state = S_IDLE;
              break;
            }
          }
          if (Serial.available()>=3) {
            //read channel
            int chan = Serial.parseInt(); //only works if a char follows the int! else 1s timeout
            //FIXME:constrain chan
            byte color = chan%3;
            while ( (Serial.available()<4) && ((millis() - starttime) < MAX_MILLIS_TO_WAIT) ) {
              if (Serial.peek() == '?') {
                //send channel state
                inB = Serial.read();
                char buf[3];
                if (chan < 505)
                  switch (color) {
                    case 0: sprintf(buf,"%03d",leds[int((chan)/3)].r); break;
                    case 1: sprintf(buf,"%03d",leds[int((chan)/3)].g); break;
                    case 2: sprintf(buf,"%03d",leds[int((chan)/3)].b); break;
                  }
                else if (chan < 512)
                  sprintf(buf,"%03d",gModeb[chan-504]);
                Serial.print(buf);
                Serial.print(DMX_RCHAR);
                state = S_IDLE;
                break;
              }
            }
            // check for set channel
            if (Serial.available()>=4) {
              if (Serial.read() == 'L') {
                Serial.setTimeout(0);
                //FIXME:constrain val
                int val = Serial.parseInt();
                Serial.setTimeout(1000);
                setChannel(chan,val);
                FastSPI_LED.show();
                Serial.write(DMX_RCHAR);
                state = S_IDLE;
              }
            }
          }
          break;
      }
    }
    LSL("Serial END")
  }
}

void runDemo() {
  //TODO: add more demos

  // one at a time
  for(int j = 0; j < 4; j++) { 
    for(int i = 0 ; i < gModeb[5]; i++ ) {
      memset(leds, 0, MAX_LEDS * 3);
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
      if (Serial.available())
        processSerial();
      delay(50);
    }
  }

  // growing/receeding bars
  for(int j = 0; j < 3; j++) { 
    memset(leds, 0, MAX_LEDS * 3);
    for(int i = 0 ; i < gModeb[5]; i++ ) {
      switch(j) { 
        case 0: leds[i].r = 255; break;
        case 1: leds[i].g = 255; break;
        case 2: leds[i].b = 255; break;
      }
      FastSPI_LED.show();
      if (Serial.available())
        processSerial();
      delay(30);
    }
    for(int i = gModeb[5]-1 ; i >= 0; i-- ) {
      switch(j) { 
        case 0: leds[i].r = 0; break;
        case 1: leds[i].g = 0; break;
        case 2: leds[i].b = 0; break;
      }
      FastSPI_LED.show();
      if (Serial.available())
        processSerial();
      delay(30);
    }
  }
  
  // Fade in/fade out
  for(int j = 0; j < 4; j++ ) { 
    memset(leds, 0, MAX_LEDS * 3);
    for(int k = 0; k < 256; k++) { 
      for(int i = 0; i < gModeb[5]; i++ ) {
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
      delay(2); //3
    }
    for(int k = 255; k >= 0; k--) { 
      for(int i = 0; i < gModeb[5]; i++ ) {
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
      delay(2); //3
    }
    if (Serial.available())
      processSerial();
  }

} // end demo

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

