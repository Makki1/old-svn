#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>


// ********************* local definitions *********************
#define DMX_CHANNELS    (3)		//Define the number of DMX values to store

#define RX_STATUS_PIN (13)
#define MAIN_PIN (2)

enum {IDLE, BREAK, STARTB, STARTADR};	//DMX states

uint8_t  gDmxState;
uint16_t DmxCount;

uint8_t  DmxRxField[DMX_CHANNELS];     //array of DMX vals (raw)
uint16_t DmxAddress;			 //start address

int ledPin = 4;			     // LED connected to digital pin 13
boolean rxled=false;

void setup()				   // run once, when the sketch starts
{
  // sets the digital pin as output
  pinMode(ledPin, OUTPUT);
  pinMode(MAIN_PIN, OUTPUT);
  bitSet(PORTB, 5);
  pinMode(RX_STATUS_PIN, OUTPUT); //RX STATUS LED pin, blinks on incoming DMX data
  digitalWrite(RX_STATUS_PIN, LOW);
  
  // Disable interrupts
  cli();

  // 250kbps baud - only works for 16MHz clock frequency. See the ATMega8 datasheet if you are using a different clock speed
  UBRR0H = 0;
  UBRR0L = 3;

  // enable rx and interrupt on complete reception of a byte
  UCSR0A |= (1<<RXC0);
  UCSR0B = (1<<RXEN0)|(1<<RXCIE0);
  UCSR0C = (1<<UMSEL01)|(3<<UCSZ00)|(1<<USBS0);

  // Enable interrupts
  sei();

  gDmxState= IDLE;

  uint8_t i;
  for (i=0; i<DMX_CHANNELS; i++)
  {
    DmxRxField[i]= 0;
  }

  DmxAddress= 10;  //Set the base DMX address. Could use DIP switches for this.
}

void loop()			   // run over and over again
{
  digitalWrite(MAIN_PIN, HIGH);   // set the LED on
  if(DmxRxField[0] >=127)
  {
    digitalWrite(RX_STATUS_PIN, HIGH);
    digitalWrite(ledPin, HIGH);   // sets the LED on
  }
  else
  {
    digitalWrite(ledPin, LOW);    // sets the LED off
  }
  digitalWrite(MAIN_PIN, LOW);   // set the LED on
}

// *************** DMX Reception ISR ****************
ISR(USART_RX_vect) {
      //digitalWrite(RX_STATUS_PIN, HIGH);
      bitSet(PORTB, 5);
      static  uint16_t DmxCount;
      uint8_t  USARTstate= UCSR0A;    //get state before data!
      uint8_t  DmxByte   = UDR0;          //get data
      uint8_t  DmxState  = gDmxState; //just load once from SRAM to increase speed
 
      if (USARTstate &(1<<FE0))               //check for break
      {
              DmxCount =  DmxAddress;         //reset channel counter (count channels before start address)
              gDmxState= BREAK;
      }
 
      else if (DmxState == BREAK)
      {
              if (DmxByte == 0)
                gDmxState= STARTB;  //normal start code detected
              else
                gDmxState= IDLE;
      }
 
      else if (DmxState == STARTB)
      {
              if (--DmxCount == 0)    //start address reached?
              {
                  DmxCount= 1;            //set up counter for required channels
                  DmxRxField[0]= DmxByte; //get 1st DMX channel of device
                  gDmxState= STARTADR;
              }
      }
 
      else if (DmxState == STARTADR)
      {
              DmxRxField[DmxCount++]= DmxByte;        //get channel
              if (DmxCount >= sizeof(DmxRxField)) //all ch received?
              {
                      gDmxState= IDLE;        //wait for next break
              }
      }
      //digitalWrite(RX_STATUS_PIN, LOW);
      bitClear(PORTB, 5);

}

