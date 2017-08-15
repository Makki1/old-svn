/* Pixelcontroller - Clone of FastSPI_Stageprofi 2013-04-28
   Implements Enttec USB Pro for OLA
   
   UNFINISHED! only a barely template
   PIN 11/13 on Uno, 51,52 on Mega2560 for SPI/LEDs
   
   See enclosed README for concept, licenses, (C), Credits etc.
   Based on many arduino-sources and arduino -rgb-mixer (C) Simon Newton
Notes2:
- might hang on EEPROM-writes!

FIXME: IMPORTANT: check for optimisations in HardwareSerial.cpp/h ! make rx_bufer uint8_t instead of int and buffer size a power of 2
http://arduino.cc/forum/index.php/topic,37874.15.html
FIXME: logarothm dimming table?!
FIXME: invert PWM-out?
MSL2041 without coil?

*********************************************************************
FIXME: Missing to RGB-Mixer in ola:
- Product Details
Product Detail IDs	PWM


   (C) 2013 Michael Markstaller / Elaborated Networks GmbH
   based on arduino-rgb-mixer (C) Simon Newton
*/
#include <EEPROM.h>

// FastSPI v20121014
#include <FastSPI_LED.h>

#define DEBUG 1
#define USE_RDM 1// wether to include RDM-support
#define USE_OW 1// wether to include 1-Wire-support; make 2200byte flash and 40byte RAM

#ifdef USE_OW
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#include "common.h"
#include "debug.h"
#include "RDMEnums.h"
#include "WidgetSettings.h"

// wiring of LEDs/colors on chip
//BLIPTRONICS: struct CRGB { unsigned char g; unsigned char b; unsigned char r; };
//DycoLED:
struct CRGB { unsigned char r; unsigned char b; unsigned char g; };
struct CRGB *leds;

//Init always max; num_leds for smooth demo/grid is set by DMX
#define MAX_LEDS 168

// usbpro device setting
const byte DEVICE_PARAMS[] = {0, 1, 0, 0, 40};
const byte DEVICE_ID[] = {1, 0};
//FIXME: put into flash/eeprom!
char DEVICE_NAME[] = "Arduino Pixelcontroller"; //=23+1
byte DEVICE_NAME_SIZE = sizeof(DEVICE_NAME);
char MANUFACTURER_NAME[] = "Open Lightning";
//char MANUFACTURER_NAME[] = "ElabNET";
byte MANUFACTURER_NAME_SIZE = sizeof(MANUFACTURER_NAME);
unsigned long DEVICE_SERIAL = { 0x0123 };
unsigned int ESTA_ID = { 0x7a70 };
//int ESTA_ID = { 0x095f };
const byte SOFTWARE_VERSION = 1;
char SOFTWARE_VERSION_STRING[] = "1.0";
char SET_SERIAL_PID_DESCRIPTION[] = "Set Serial#";
//char SUPPORTED_LANGUAGE[] = "en";

/* SpectrumAnalyzer */
//For spectrum analyzer shield, these three pins are used.
//You can move pinds 4 and 5, but you must cut the trace on the shield and re-route from the 2 jumpers. 
const byte spectrumReset=5;
const byte spectrumStrobe=4;
const byte spectrumAnalog=0;  //0 for left channel, 1 for right. A1 is connected to optional ACS714!

/* 1-Wire */
#ifdef USE_OW
const byte OWPIN = 7;
OneWire oneWire(OWPIN);
DallasTemperature owsensors(&oneWire);
#endif

/* Globals 
  Memory should be smthg like 1800 - 600 - 512 = 600 bytes free on Uno/Mega328
*/
byte gModeb[] = { 0, 0, 0, 0, 0, 100, 5, 4 }; //Defaults: PWM-pins R/G/B/W, Mode (5), numleds, gridX, gridY
//FIXME: move numleds, x,y to EEPROM/Widgetsettings
const byte ID_PIN = 8; //identify/power-LED
const byte SEED_PIN = 2; //unconnected analog-in for seed
byte led_state = 1;
volatile bool button_state = 0;

byte recv_state = U_PRE_SOM;
byte message[600];
int expected_size = 0;
int data_offset = 0;
byte label = 0;
unsigned int rdm_checksum = 0;

/* Temp vars / debug */
unsigned long lastcheck = millis(); //only for debug
unsigned long data_last = millis(); //data-watchdog
int democounter = 0;
int msgfails = 0;

void buttonPress () {
    button_state = 1;
}

void setup()
{
  //wdt_enable(WDTO_2S);
  DEBUG_INIT LL LL
  DEBUG_PRINT("Debug-Port enabled, setup")
  LS("Code: ") LS(__DATE__ " " __TIME__) LS(" Free: ") LV(freeRam()) LL

/***********************************************************************************************************/
/* START just testing */
/***********************************************************************************************************/
LSL("Test start")
LSL("Test end")
/* END testing */
/***********************************************************************************************************/

  //FIXME: unused pins low, output-pins?
  //TODO: EEprom save/restore? + random serial-generator on first start, blackout-save?
  //TODO: log. dimming curve and internal fading as Personality/Option-channel?
  //TODO: Temp, Voltage, Power-Sensors, Button/switch/jumper demo or capacitive button
  
  FastSPI_LED.setLeds(MAX_LEDS);
  // LPD6803, 5bit/color for Bliptronics, DycoLED, MagiarLED etc.
  FastSPI_LED.setChipset(CFastSPI_LED::SPI_LPD6803);
  //FastSPI_LED.setPin(PIN); //only needed for UCS1803 or TM1809 
  
  FastSPI_LED.init();
  FastSPI_LED.start();
  leds = (struct CRGB*)FastSPI_LED.getRGBData();
  // clear all
  FastSPI_LED.show();
  LS("INIT: After leds: ") LV(freeRam()) LL
  
  pinMode(3, OUTPUT);   // set the PWM-pins as output
  pinMode(6, OUTPUT);   // set the PWM-pins as output
  pinMode(9, OUTPUT);   // set the PWM-pins as output
  pinMode(10, OUTPUT);   // set the PWM-pins as output
  pinMode(ID_PIN, OUTPUT);
  pinMode(2, INPUT);    // save-button, use INT0?
  digitalWrite(2, HIGH);  // Turn on internal Pull-Up Resistor
  attachInterrupt(0, buttonPress, CHANGE); //PIN2 / INT0

/*
  //Setup pins to drive the spectrum analyzer. 
  pinMode(spectrumReset, OUTPUT);
  pinMode(spectrumStrobe, OUTPUT);

  //Init spectrum analyzer
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
*/

#ifdef USE_OW
  LSL("OW-Init start")
  //FIXME: SearchRom fails sometimes on start, retry cyclic?
  //This is a quirk! 
  //On DSO it looks like the internal pulldown is not set correctly - sometimes(?)
  pinMode(OWPIN,OUTPUT);
  digitalWrite(OWPIN,LOW);
  owsensors.begin();
  owsensors.setWaitForConversion(false); // makes it async
  owsensors.requestTemperatures();
/* broken, maybe too early/fast
  byte owscount;
  DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
  owscount = owsensors.getDeviceCount();
  // Loop through each device, print out address
  for(int i=0;i<owscount; i++) {
    // Search the wire for address
    if(owsensors.getAddress(tempDeviceAddress, i)) {
      LS("Found device ") LV(i) LS(" address: ") 
      for (uint8_t j = 0; j < 8; j++) {
        LVH(tempDeviceAddress[j]);
      }
      LL
    }
  }
  LS("C: ") LV(owsensors.getDeviceCount()) LL
  LSL("OW-Init end: ") LV(freeRam()) LL // takes 114ms@1, 210ms@2
*/
#endif

  // initialize the serial communication:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  LS("INIT: After serial: ") LV(freeRam()) LL
  
#if defined(__AVR_ATmega2560__)
  analogReference(INTERNAL1V1);
#else //Atmega 168/328
  analogReference(INTERNAL);
#endif
  LS("Seed:") LV(analogRead(0)) LL
  // Initialize random seed, A0 must be unconnected!
  randomSeed(analogRead(SEED_PIN)); //FIXME: only if seed needed? (once)
  WidgetSettings.Init();

  /* TODO: ? init PWMs based on personality?
  byte personality = WidgetSettings.Personality();
  LS("INIT: After WidgetSettings: ") LV(freeRam()) LL
  */

} //setup


void loop() {
  //wdt_reset();
  //FIXME: #########################################################
  //FIXME: make own delay_ms()-function for demo/ow which calls serial!!!
  //FIXME: #########################################################
  processSerial();

  //Perform outstanding EEPROM-writes
  WidgetSettings.PerformWrite();
  /*
  if (WidgetSettings.PerformWrite()) {
    rdm_handler.QueueSetDeviceLabel();
  }
  */
  
  //TODO: PIN-out?
  //TODO EEpromsave-button?
  //TODO: Tempsensor, read Voltage/Power?
  FastSPI_LED.show(); // not required, though useful to output cyclic if pixels connect/disconnected here..

  //FIXME: switch different demos, spectrum-analyzer
  //box, line
  // running the demo breaks receiver! This is why the lower channels are for setup.
  if (gModeb[4] == 2) {
    runDemo();
//    democounter++;
  //FIXME: for-loop
  } else if (gModeb[4] == 0) {
    analogWrite(3, gModeb[0]);
    analogWrite(6, gModeb[1]);
    analogWrite(9, gModeb[2]);
    analogWrite(10, gModeb[3]);
  }
  //FIXME: unknown, just to test..
  //Serial.flush();


  if (recv_state != U_PRE_SOM && (millis() - data_last) > 2000) {
    LSL("Serial watchdog-Timeout!!")
    ReturnRDMErrorResponse(RDM_STATUS_FAILED);
    recv_state=U_PRE_SOM; label=0; expected_size=0; data_offset=0;
  }

#ifdef DEBUG
  if (millis() - lastcheck > 2000) {
    LS("L:") LV(millis()) LS(":State:")
    LV(recv_state) LS(" Label:") LV(label) LS(" Ex:") LV(expected_size) LS( " of:") LV(data_offset) LS (" mfail:") LV(msgfails) LL
    /*
    if (led_state)
      led_state = 0;
    else
      led_state = 1;
    digitalWrite(ID_PIN, led_state);
    */
    
#ifdef USE_OW
    LSL("owtemp: ") LV(owsensors.getTempCByIndex(0)) LS(" , ") LV(owsensors.getTempCByIndex(1)) LL //6ms
    unsigned char sname[8];
    owsensors.getAddress(sname, 0);
    LV(owsensors.getDeviceCount()) Serial3.println(*sname);
    LSL("req-new-read") //110-130ms
    owsensors.requestTemperatures(); //5ms
    LSL("OWend")
    LS("A2:") LV(analogRead(2)) LL
#endif

    lastcheck = millis();
#if defined(__AVR_ATmega2560__)
    //accept some debug-commands on Serial3
    if (Serial3.available()) {
      byte tmp = Serial3.read();
      switch (tmp) {
        case 'D': //print Debug, dump values etc..
        case 'V': //print Debug, dump values etc..
          LS("Code: ") LS(__DATE__ " " __TIME__) LS(" Free: ") LV(freeRam()) LL
          break;
        case 'E': //erase eeprom!
          LSL("Erasing EEPROM!")
          for (int i = 0; i < 1024; i++)
            EEPROM.write(i, 0xFF);
          break;
        case 'O':
          LSL("OW re-Init")
          pinMode(OWPIN,OUTPUT);
          digitalWrite(OWPIN,LOW);
          delay(500);
          owsensors.begin();
          owsensors.setWaitForConversion(false); // makes it async
          break;
      }
    }
#endif //mega2560
  }
  if (button_state) {
    LS("Button pressed!") LL
    button_state = 0;
  }
#endif //debug
} //loop



void processUSBmessage() {
  LL LSL("Received full USB message!")
  LS(" Label, Size: ") LVH(label) LV(expected_size)
#ifdef DEBUG2
  for (int i=0;i<expected_size;i++) { LS(" D:") LV(i) LS(":") LVH(message[i]) } 
  LL
#endif
  LL
  int tmp;
  switch (label) {
    case PARAMETERS_LABEL:
      LSL("Getparams")
      //Get parameters, data0+1 = LSB/MSB of user-config-size
      //FIXME: Header/Footer??
      Serial.write(PARAMETERS_LABEL);
      Serial.write(DEVICE_PARAMS,sizeof(DEVICE_PARAMS));
      break;
    case DMX_DATA_LABEL:
      LSL("SetDMX")
      //FIXME: use copy! setRGBData
      for (int i=1;i < expected_size;i++) {
        //LS("C:") LV((message[0])+i-1) LS(" to") LVH(message[i])
        setChannel((message[0])+i-1,message[i]);
      }
      FastSPI_LED.show();
      break;
    case SERIAL_NUMBER_LABEL:
      LSL("GetSerial")
      //FIXME: get from eeprom/generated
      Serial.write(0x7E);
      Serial.write(SERIAL_NUMBER_LABEL);
      Serial.write(4); //size
      Serial.write(0);
      Serial.write(WidgetSettings.SerialNumber() & 0xFF);
      Serial.write((WidgetSettings.SerialNumber() >> 8) & 0xFF);
      Serial.write((WidgetSettings.SerialNumber() >> 16) & 0xFF);
      Serial.write((WidgetSettings.SerialNumber() >> 24) & 0xFF);
      Serial.write(0xE7);
      break;
    case NAME_LABEL:
      LSL("GetName")
      tmp = sizeof(DEVICE_ID) + sizeof(DEVICE_NAME);
      Serial.write(0x7E);
      Serial.write(NAME_LABEL);
      Serial.write(tmp);
      Serial.write(tmp >> 8);
      Serial.write(DEVICE_ID, sizeof(DEVICE_ID));
      Serial.write((byte*) DEVICE_NAME, sizeof(DEVICE_NAME));
      Serial.write(0xE7);
      break;
    case MANUFACTURER_LABEL:
      LSL("GetManu")
      tmp = sizeof(ESTA_ID) + sizeof(MANUFACTURER_NAME);
      Serial.write(0x7E);
      Serial.write(MANUFACTURER_LABEL);
      Serial.write(tmp);
      Serial.write(tmp >> 8);
      // ESTA ID is sent in little endian format
      Serial.write(ESTA_ID);
      Serial.write(ESTA_ID >> 8);
      Serial.write((byte*) MANUFACTURER_NAME, sizeof(MANUFACTURER_NAME));
      Serial.write(0xE7);
      break;
    case RDM_LABEL:
//      TODO: LED..
//      led_state = !led_state;
//      digitalWrite(LED_PIN, led_state);
      handleRDMMessage();
      break;
    default:
      LS("Unsupported message-label:") LV(label) LL
      break;
  }
}

void handleRDMMessage() {
#ifdef USE_RDM
  LSL("RDM-Handler: ")
#ifdef DEBUG
  for (int i=0;i<expected_size;i++) { LVH(message[i]) } 
  LL
#endif
  if (expected_size < MINIMUM_RDM_PACKET_SIZE ||
      message[0] != START_CODE || 
      message[1] != SUB_START_CODE ||
      message[2] != expected_size - 2) {
    ReturnRDMErrorResponse(RDM_STATUS_FAILED);
    return;
  }

  if (!verifyRDMChecksum()) {
    ReturnRDMErrorResponse(RDM_STATUS_FAILED_CHECKSUM);
    return;
  }

  // if this is broadcast or vendorcast, we don't return a RDM message
  bool is_broadcast = true;
  for (int i = 5; i <= 8; ++i) {
    is_broadcast &= (message[i] == 0xff);
  }
  if (is_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }
  
  unsigned long target_serial = (unsigned long)message[5] << 24 | (unsigned long)message[6] << 16 | (unsigned long)message[7] << 8 | (unsigned long)message[8];
  if (!(WidgetSettings.EstaId() == (message[3] << 8) + message[4]) ||
      !(WidgetSettings.SerialNumber() == target_serial)) {
    ReturnRDMErrorResponse(RDM_STATUS_INVALID_DESTINATION);
    return;
  }

  // check the command class
  byte command_class = message[20];
  if (command_class != GET_COMMAND && command_class != SET_COMMAND) {
    ReturnRDMErrorResponse(RDM_STATUS_INVALID_COMMAND);
    return;
  }

  // check sub devices
  unsigned int sub_device = (message[18] << 8) + message[19];
  if (sub_device != 0 && sub_device != 0xffff) {
    // respond with nack
    if (is_broadcast) //FIXME: doppelt..
      ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    else
      RDMNackOrBroadcast(is_broadcast,NR_SUB_DEVICE_OUT_OF_RANGE);      
      //ReturnRDMErrorResponse(NR_SUB_DEVICE_OUT_OF_RANGE); //should be NACK?
    return;
  }
  if (sub_device) {
    RDMSendNack(NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  unsigned int param_id = (message[21] << 8) + message[22];

  if (command_class == GET_COMMAND) {
    RDMGetPidHandler(param_id);
  } else if (command_class == SET_COMMAND) {
    RDMSetPidHandler(param_id);
  } else {
    RDMNackOrBroadcast(0, NR_UNKNOWN_PID); 
  }

  return;

#else
//TODO: undefined - return RDM_FAILED or so..
#endif
} //handleRDMMessage

//TODO: exclude/ifdef all RDM-stuff
bool RDMSetPidHandler(unsigned int pid) {
  LS("RDMSetPidHandler ") LVH(pid) LL
  switch (pid) {
    //FIXME:? check value, subdevice, broadcast really needed again?
    case PID_DEVICE_LABEL:
      {
        // check for invalid size or value
        if (message[23] > MAX_LABEL_SIZE) {
          RDMSendNack(NR_FORMAT_ERROR);
          return false;
        }
        WidgetSettings.SetDeviceLabel((char*) message + 24,message[23]);
        StartRDMAckResponse(0);
        EndRDMResponse();
      }
      break;
    case PID_IDENTIFY_DEVICE:
      StartRDMAckResponse(0);
      EndRDMResponse();
      digitalWrite(ID_PIN,message[24]);
      break;
    //LAMP_HOURS, DEVICE_HOURS, LAMP_STRIKES, DISPLAY_LEVEL, REAL_TIME_CLOCK
    //PRESETS
    case PID_MANUFACTURER_SET_SERIAL:
      {
        uint32_t new_serial_number = 0;
        for (byte i = 0; i < 4; ++i) {
          new_serial_number = new_serial_number << 8;
          new_serial_number += message[24 + i];
        }
        WidgetSettings.SetSerialNumber(new_serial_number);
        StartRDMAckResponse(0);
        EndRDMResponse();
      }
      break;
  }
} //RDMSetPidHandler

bool RDMGetPidHandler(unsigned int pid) {
  //TODO: handle all know PIDs
  LS("RDMGetPidHandler ") LVH(pid) LL
  switch (pid) {
    case PID_QUEUED_MESSAGE:
      StartRDMCustomResponse(RDM_RESPONSE_ACK,
        0, GET_COMMAND_RESPONSE, PID_STATUS_MESSAGES);
      EndRDMResponse();
      break;
    case PID_MANUFACTURER_LABEL:
      StartRDMAckResponse(MANUFACTURER_NAME_SIZE);
      RDMSendStringRequest(MANUFACTURER_NAME,MANUFACTURER_NAME_SIZE);
      EndRDMResponse();
      break;
    case PID_DEVICE_LABEL:
      {
        char device_label[MAX_LABEL_SIZE];
        byte size = WidgetSettings.DeviceLabel(device_label, sizeof(device_label));
        StartRDMAckResponse(size);
        RDMSendStringRequest(device_label, size);
        EndRDMResponse();
      }
      break;
    case PID_SUPPORTED_PARAMETERS:
      {
        LS("Supported PIDs:")
        StartRDMAckResponse(sizeof(RDM_SUPPORTED_PIDS));
        for (byte i = 0; i < sizeof(RDM_SUPPORTED_PIDS)/2; i++) {
          RDMSendIntAndChecksum(RDM_SUPPORTED_PIDS[i]);
          LVH(RDM_SUPPORTED_PIDS[i])
        }
        LL
        EndRDMResponse();
      }
      break;
    case PID_PARAMETER_DESCRIPTION:
      {
        unsigned int param_id = (((unsigned int) message[24] << 8) + message[25]);
        //FIXME: describe manufacturer-params!
        switch (param_id) {
          case PID_MANUFACTURER_SET_SERIAL:
            StartRDMAckResponse(0x14 + sizeof(SET_SERIAL_PID_DESCRIPTION) - 1);
            RDMSendIntAndChecksum(param_id);
            RDMSendByteAndChecksum(4);  // pdl size
            RDMSendByteAndChecksum(0x03);  // data type, uint8
            RDMSendByteAndChecksum(0x02);  // command class, set only
            RDMSendByteAndChecksum(0);  // type, ignored
            RDMSendByteAndChecksum(0);  // unit, none
            RDMSendByteAndChecksum(0);  // prefix, none
            RDMSendLongAndChecksum(1);  // min
            RDMSendLongAndChecksum(0xfffffffe);  // max
            RDMSendLongAndChecksum(WidgetSettings.SerialNumber());  // default
            for (unsigned int i = 0; i < sizeof(SET_SERIAL_PID_DESCRIPTION) - 1; ++i)
              RDMSendByteAndChecksum(SET_SERIAL_PID_DESCRIPTION[i]);
            EndRDMResponse();
            break;
          case PID_MANUFACTURER_PIXELS:
            StartRDMAckResponse(0x14 + sizeof(SET_SERIAL_PID_DESCRIPTION) - 1);
            RDMSendIntAndChecksum(param_id);
            RDMSendByteAndChecksum(1);  // pdl size
            RDMSendByteAndChecksum(0x03);  // data type, uint8
            RDMSendByteAndChecksum(0x03);  // command class, set only
            RDMSendByteAndChecksum(0);  // type, ignored
            RDMSendByteAndChecksum(0);  // unit, none
            RDMSendByteAndChecksum(0);  // prefix, none
            RDMSendLongAndChecksum(0);  // min
            RDMSendLongAndChecksum(168);  // max
            RDMSendLongAndChecksum(WidgetSettings.SerialNumber());  // default
            for (unsigned int i = 0; i < sizeof(SET_SERIAL_PID_DESCRIPTION) - 1; ++i)
              RDMSendByteAndChecksum(SET_SERIAL_PID_DESCRIPTION[i]);
            EndRDMResponse();
            break;
        
          default:
            RDMSendNack(NR_DATA_OUT_OF_RANGE);
            break;
        }
      }
      break;
    case PID_DEVICE_INFO:
      {
        StartRDMAckResponse(19);
        RDMSendIntAndChecksum(256);  // protocol version
        RDMSendIntAndChecksum(2);  // device model
        RDMSendIntAndChecksum(0x0509);  // product category
        RDMSendLongAndChecksum(SOFTWARE_VERSION);  // software version
      
        byte personality = WidgetSettings.Personality();
        //SendIntAndChecksum(rdm_personalities[personality - 1].slots);
        RDMSendIntAndChecksum(512); //FIXME: dummy
        // current personality
        RDMSendByteAndChecksum(personality); //current personality + 1
        //RDMSendByteAndChecksum(sizeof(rdm_personalities) / sizeof(rdm_personality));
        RDMSendByteAndChecksum(1); //FIXME: # of personalities avail
        // DMX Start Address
        RDMSendIntAndChecksum(WidgetSettings.StartAddress());
        RDMSendIntAndChecksum(0);  // Sub device count
        RDMSendByteAndChecksum(owsensors.getDeviceCount());  //FIXME: Sensor Count
        EndRDMResponse();
      }
      break;
    //case PID_PRODUCT_DETAIL_ID_LIST:
    case PID_DEVICE_MODEL_DESCRIPTION:
      StartRDMAckResponse(DEVICE_NAME_SIZE);
      RDMSendStringRequest(DEVICE_NAME, DEVICE_NAME_SIZE);
      EndRDMResponse();
    break;
    case PID_FACTORY_DEFAULTS:
      StartRDMAckResponse(1);
      RDMSendByteAndChecksum(WidgetSettings.DevicePowerCycles() > 1 ? 0 : 1);
      EndRDMResponse();
      break;
    case PID_SOFTWARE_VERSION_LABEL:
      StartRDMAckResponse(sizeof(SOFTWARE_VERSION_STRING));
      RDMSendStringRequest(SOFTWARE_VERSION_STRING, sizeof(SOFTWARE_VERSION_STRING));
      EndRDMResponse();
      break;
    case PID_DMX_PERSONALITY:
      StartRDMAckResponse(2);
      RDMSendByteAndChecksum(WidgetSettings.Personality()); //current personality + 1
      RDMSendByteAndChecksum(1); //FIXME: # of personalities avail
      EndRDMResponse();
      break;
    case PID_DMX_PERSONALITY_DESCRIPTION:
      {
        LSL("FIXME! Personality descr")
        char desc[] = "Pers";
        byte size = sizeof(desc);
        StartRDMAckResponse(size + 3);
        RDMSendByteAndChecksum(1); //FIXME: the one requested
        RDMSendIntAndChecksum(512); //FIXME: DMX footprint
        RDMSendStringRequest(desc,sizeof(desc));
        EndRDMResponse();
      }
      break;
    case PID_DMX_START_ADDRESS:
      StartRDMAckResponse(2);
      RDMSendIntAndChecksum(WidgetSettings.StartAddress());
      EndRDMResponse();
      break;
    case PID_SENSOR_DEFINITION:
      {
        //FIXME: Move this whole stuff to WidgetSettings. 
        //S0 = freemem
        //A1 = ACS712-05 =(Vout-(Vcc/2))/0.185 - see Vcc for Vout!
        //A2 = Vcc = 5,9737*aRead/1000 at 10k/2k2
        //A3 = Vext = 33,6699*aRead/1000 at 10k/330
        //S4- = 1-Wire, ignore 0xFF
      //message[24] = sensor-number 0 based
        //SendNack(received_message, NR_DATA_OUT_OF_RANGE);
      unsigned char sname[8];
      uint8_t sidx = message[24];
      owsensors.getAddress(sname, sidx);
      //String sstr = String(sname,HEX);
      LS("SensorDefGet ") LVH(sidx) LV(sname[0]) LL
      StartRDMAckResponse(13 + sizeof(sname) - 1);
      RDMSendByteAndChecksum(message[24]);
      RDMSendByteAndChecksum(0x00);  // type: temperature
      RDMSendByteAndChecksum(1);  // unit: C
      RDMSendByteAndChecksum(1);  // prefix: deci
      RDMSendIntAndChecksum(0);  // range min
      RDMSendIntAndChecksum(1500);  // range max
      RDMSendIntAndChecksum(100);  // normal min
      RDMSendIntAndChecksum(400);  // normal max
      RDMSendByteAndChecksum(0);  // recorded value support
      for (unsigned int i = 0; i < sizeof(sname) - 1; ++i)
        RDMSendByteAndChecksum('A'); //FIXME: should be ID..
        //crap RDMSendByteAndChecksum((char) *sname+i);
      LSL("Sensor: ") Serial3.print(*sname,HEX); LVH(sidx)
      EndRDMResponse();
      }
      break;
    case PID_SENSOR_VALUE:
      StartRDMAckResponse(9);
      RDMSendByteAndChecksum(message[24]);
      //FIXME: ignore -127 and 85 and send back last value or start conversion here and return last value?
      RDMSendIntAndChecksum(int(owsensors.getTempCByIndex(message[24]) * 10));  // current
      RDMSendIntAndChecksum(0);  // lowest
      RDMSendIntAndChecksum(0);  // highest
      //RDMSendIntAndChecksum(WidgetSettings.SensorValue());  // recorded
      RDMSendIntAndChecksum(10);  // recorded //FIXME: error-count or smthg?
      EndRDMResponse();
      break;
/* FIXME:
  // sensors
  PID_SENSOR_DEFINITION = 0x0200,
  PID_SENSOR_VALUE = 0x0201,
//  PID_RECORD_SENSORS = 0x0202,
*/
// SLOT_INFO??
    case PID_DEVICE_HOURS:
      StartRDMAckResponse(4);
      RDMSendLongAndChecksum(int(millis()/1000/3600)); //actually it should not reset? this is just the "uptime"
      EndRDMResponse();
      break;
// LAMP_HOURS? record every hour makes 24*365=8760 writes/year
    case PID_DEVICE_POWER_CYCLES:
      StartRDMAckResponse(4);
      RDMSendLongAndChecksum(WidgetSettings.DevicePowerCycles()); //actually it should not reset? this is just the "uptime"
      EndRDMResponse();
      break;
    case PID_IDENTIFY_DEVICE:
      StartRDMAckResponse(1);
      RDMSendByteAndChecksum(digitalRead(ID_PIN)); //FIXME: WidgetSettings.Identify() ?
      EndRDMResponse();
      break;
    default:
      RDMNackOrBroadcast(0, NR_UNKNOWN_PID);
      LS("RDMPidGetHandler unsupported PID: ") LVH(pid) LL
      return false;
      break;
  }
  //TODO: send error here, return false on error
  return true;
} //RDMPidGetHandler


void RDMSendStringRequest(char *label, byte label_size) {
  //message is global, label is string, label_size is length
  //StartRDMResponse(RDM_RESPONSE_ACK, label_size);
  for (unsigned int i = 0; i < label_size; ++i)
    RDMSendByteAndChecksum(label[i]);
  //EndRDMResponse();
}

void RDMSendByteAndChecksum(byte b) {
  rdm_checksum += b;
  Serial.write(b);
}
void RDMSendIntAndChecksum(int i) {
  RDMSendByteAndChecksum(i >> 8);
  RDMSendByteAndChecksum(i);
}
void RDMSendLongAndChecksum(unsigned long l) {
  RDMSendIntAndChecksum(l >> 16);
  RDMSendIntAndChecksum(l);
}


//FIXME: ?useless, combine with customresponse?
void StartRDMAckResponse(unsigned int param_data_size) {
  StartRDMResponse(RDM_RESPONSE_ACK, param_data_size);
}
void StartRDMResponse(byte response_type, unsigned int param_data_size) {
  int pid = message[21];
  pid = (pid << 8) + message[22];

  StartRDMCustomResponse(
      response_type,
      param_data_size,
      message[20] == GET_COMMAND ?
        GET_COMMAND_RESPONSE : SET_COMMAND_RESPONSE,
      pid);
}
void StartRDMCustomResponse(byte response_type,
                                    unsigned int param_data_size,
                                    byte command_class,
                                    int pid) {
  // set the checksum to 0
  //unsigned int 
  rdm_checksum = 0;
  // size is the rdm status code, the rdm header + the param_data_size
  UsbSendMessageHeader(RDM_LABEL,
                              1 + MINIMUM_RDM_PACKET_SIZE + param_data_size);
  RDMSendByteAndChecksum(RDM_STATUS_OK);
  RDMSendByteAndChecksum(START_CODE);
  RDMSendByteAndChecksum(SUB_START_CODE);
  RDMSendByteAndChecksum(MINIMUM_RDM_PACKET_SIZE - 2 + param_data_size);

  // copy the src uid into the dst uid field
  RDMSendByteAndChecksum(message[9]);
  RDMSendByteAndChecksum(message[10]);
  RDMSendByteAndChecksum(message[11]);
  RDMSendByteAndChecksum(message[12]);
  RDMSendByteAndChecksum(message[13]);
  RDMSendByteAndChecksum(message[14]);

  // add our UID as the src, the ESTA_ID & fields are reversed
  RDMSendIntAndChecksum(WidgetSettings.EstaId());
  RDMSendLongAndChecksum(WidgetSettings.SerialNumber());

  RDMSendByteAndChecksum(message[15]);  // transaction #
  RDMSendByteAndChecksum(response_type);  // response type
  RDMSendByteAndChecksum(0);  // message count //FIXME: ??? m_message_count related to queued messages - not implemented

  // sub device
  RDMSendByteAndChecksum(message[18]);
  RDMSendByteAndChecksum(message[19]);

  // command class
  RDMSendByteAndChecksum(command_class);

  // param id, we don't use queued messages so this always matches the request
  RDMSendByteAndChecksum(pid >> 8);
  RDMSendByteAndChecksum(pid);
  RDMSendByteAndChecksum(param_data_size);
}

/**
 * Send a Nack response
 * @param nack_reason the NACK reasons
 */
void RDMSendNack(rdm_nack_reason nack_reason) {
  StartRDMResponse(RDM_RESPONSE_NACK, 2);
  RDMSendIntAndChecksum(nack_reason);
  EndRDMResponse();
}


/**
 * Send a NACK or a 'was broadcast' response.
 */
void RDMNackOrBroadcast(bool was_broadcast,rdm_nack_reason nack_reason) {
  if (was_broadcast)
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  else
    RDMSendNack(nack_reason);
}


void UsbSendMessageHeader(byte label, int size) {
  Serial.write(0x7E);
  Serial.write(label);
  Serial.write(size);
  Serial.write(size >> 8);
}

void EndRDMResponse() {
  Serial.write(rdm_checksum >> 8);
  Serial.write(rdm_checksum);
  Serial.write(0xE7);
}

void ReturnRDMErrorResponse(byte error_code) {
  LS("RDM Err: ") LVH(error_code) LL
  UsbSendMessageHeader(RDM_LABEL, 1);
  Serial.write(error_code);
  Serial.write(0xE7);
}

bool verifyRDMChecksum() {
  // don't checksum the checksum itself (last two bytes)
  unsigned int checksum = 0;
  for (int i = 0; i < expected_size - 2; i++)
    checksum += message[i];

  byte checksum_offset = message[2];
  return (checksum >> 8 == message[checksum_offset] &&
          (checksum & 0xff) == message[checksum_offset + 1]);
}

void processSerial() {
  while (Serial.available()) {
    int data = Serial.read();
    //LS("Sin:") LVH(data) LL
    //TODO: Simple ASCII-Mode?
    switch (recv_state) {
      case U_PRE_SOM:
        if (data == 0x7E) {
          recv_state = U_GOT_SOM;
            if (gModeb[4] == 2) {
              gModeb[4] = 0; //demo off
              LSL("Recv->Demo off");
            }
        // ASCII-commands for testing..
        } else if (data == 'V') {
          Serial.println(freeRam()); //6199 with ow etc. but without RDMHandler
        } else if (data == 'D') { // toggle demo
          if (gModeb[4] == 0)
            gModeb[4] = 2;
        } else {
          //LSL("SPRE:dunno: ") LVH(data)
        }
        break;
      case U_GOT_SOM:
        label = data;
        recv_state = U_GOT_LABEL;
        break;
      case U_GOT_LABEL:
        data_offset = 0;
        expected_size = data;
        recv_state = U_GOT_DATA_LSB;
        break;
      case U_GOT_DATA_LSB:
        expected_size += (data << 8);
        if (expected_size == 0) {
          recv_state = U_WAITING_FOR_EOM;
        } else {
          recv_state = U_IN_DATA;
        }
        break;
      case U_IN_DATA:
        //broken with data > 64byte!
        message[data_offset] = data;
        data_offset++;
        data_last = millis();
//        LS(" Data#") LV(data_offset) LS(" : ") LV(data)
        if (data_offset == expected_size) {
          recv_state = U_WAITING_FOR_EOM;
        }
        break;
      case U_WAITING_FOR_EOM:
        if (data == 0xE7) {
          // this was a valid packet, act on it
          processUSBmessage();
        } else {
          LSL("Timeout waiting for EOM")
          msgfails++;
        }
        recv_state = U_PRE_SOM;
    }
  }
}

void setChannel(int chan, byte val) {

  byte color = chan % 3;
  if (chan < 504 && gModeb[4] == 0) //only set single channels if in DMX-mode
    switch (color) {
        case 0: leds[int(chan/3)].r = val; break;
        case 1: leds[int(chan/3)].g = val; break;
        case 2: leds[int(chan/3)].b = val; break;
    }
    else if (chan < 512)
        gModeb[chan-504] = val;
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
      delay(20); //20
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
      delay(10); //10
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
      delay(10); //10
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
      delay(1); //3
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
      delay(1); //3
    }
    if (Serial.available())
      processSerial();
  }

} // end demo

/* some IMPORTANT hints: http://playground.arduino.cc/Main/CorruptArrayVariablesAndMemory

avr-objdump -h /tmp/build9122930650034551321.tmp/FastSPI_usbpro.cpp.elf
avr-size /tmp/build9122930650034551321.tmp/FastSPI_usbpro.cpp.elf
avr-size -A --mcu=atmega328 FastSPI_usbpro.cpp.o
*/

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

