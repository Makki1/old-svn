// FastSPI v20121014
#include <FastSPI_LED.h>

// free RAM 1240 @170, 1690 @20
const int NUM_LEDS = 20;

// Sometimes chipsets wire in a backwards sort of way
//struct CRGB { unsigned char b; unsigned char r; unsigned char g; };
//struct CRGB { unsigned char r; unsigned char g; unsigned char b; };
struct CRGB { unsigned char g; unsigned char b; unsigned char r; };
struct CRGB *leds;

//Not sure we need this?
#define PIN 7
#define DEBUG 1
boolean DEMO = 1; // Enable demo until first serial recv

void setup()
{
  FastSPI_LED.setLeds(NUM_LEDS);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_SM16716);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_TM1809);
  FastSPI_LED.setChipset(CFastSPI_LED::SPI_LPD6803);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_HL1606);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_595);
  //FastSPI_LED.setChipset(CFastSPI_LED::SPI_WS2801);

  FastSPI_LED.setPin(PIN);
  
  FastSPI_LED.init();
  FastSPI_LED.start();
  leds = (struct CRGB*)FastSPI_LED.getRGBData(); 

  // initialize the serial communication:
  Serial.begin(38400);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print(F("Up and running, this is from flash! codebase: "));
  Serial.println(F(__DATE__));
  Serial.println(freeRam());
}

void loop() {
  if (Serial.available() > 0) {
    byte inB = Serial.read();
    Serial.print("Was? 0x");
    Serial.println(inB, HEX);
    //peek and try to read full msg with timeout
    switch (inB) {
      case 0xFF: //up to 255 DMX bytes follow
        break;
      case 0xE0: // Single LOW channel with response G (undocumented?)
        break;
      case 0xE1: // Single HIGH channel with response G (undocumented?)
        break;
      case 0xE2: // Single LOW channel -fast mode
        break;
      case 0xE3: // Single LOW channel -fast mode
        break;
    }
    //just testing - output
    memset(leds, 0, NUM_LEDS * 3); //clear all pixels
    leds[inB-0x30].r = 255;
    FastSPI_LED.show();
    Serial.println(freeRam()); 
    DEMO = 0;
  }
  
  if (DEMO) {
    runDemo();
  }
  
}

void runDemo() {
  //TODO: add more demos
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
      delay(50);
    }
  }

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
      delay(20);
    }
    for(int i = NUM_LEDS-1 ; i >= 0; i-- ) {
      switch(j) { 
        case 0: leds[i].r = 0; break;
        case 1: leds[i].g = 0; break;
        case 2: leds[i].b = 0; break;
      }
      FastSPI_LED.show();
      delay(20);
    }
  }
  
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
      delay(2); //3
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
      delay(2); //3
    }
  }
}

#ifdef DEBUG
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
#endif

