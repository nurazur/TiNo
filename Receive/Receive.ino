/* RFM69CW Receiver Sketch for Serial communication, i.e. RaspberryPI

patially based on work of Nathan Chantrell
modified by meigrafd @2013 - for UART on RaspberryPI
modified by nurazur nurazur@gmail.com @2018

This sketch can receive general packet formats, but the first 3 bytes are fixed TARGETID, NODEID and FLAGS

********************************************************************************
License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


Licence can be viewed at                               
http://www.fsf.org/licenses/gpl.txt                    

Please maintain this license information along with authorship
and copyright notices in any redistribution of this code
*********************************************************************************
*/

#include <RFM69.h>
#include <RFM69registers.h>
#include <avr/sleep.h>
#include <SoftwareSerial.h>
#include "Mac.h"
#include "configuration_dist.h"
#include "calibrate_dist.h"

#define SKETCHNAME "Receive.ino Ver 23/11/2018"
#define BUILD 7
//------------------------------------------------------------------------------
// You will need to initialize the radio by telling it what ID it has and what network it's on
// The Network ID takes values from 0-255
// By default the SPI-SS line used is D10 on Atmega328. You can change it by calling .SetCS(pin) where pin can be {8,9,10}
#define THIS_NODEID       22  //node ID used for this unit
#define NETWORKID        210  //the network ID
#define ACK_TIME        2000  // # of ms to wait for an ack
#define SERIAL_BAUD     38400
#define TXPOWER           25 // 7 = -18 + 25,  this is 7 dBm.
#define F_DEV_STEPS     0  

//------------------------------------------------------------------------------
// PIN-Konfiguration 
//------------------------------------------------------------------------------
// UART pins
#define RXPIN 0 
#define TXPIN 1 // pin der an RXD vom PI geht.
// LED pin
#define LEDpin 8 // D8, PA2 - set to 0 to disable LED


//encryption is OPTIONAL
// to enable encryption you will need to:
// - provide a 16-byte encryption KEY (same on all nodes that talk encrypted)
// - set the varable ENCRYPTION_ENABLE to 1 in the EEPROM, at runtime in Cfg.EncryptionEnable
#define KEY  "TheQuickBrownFox"


typedef struct {
  uint8_t node;      // the node I want to trigger
  uint8_t port;      // the port I want to trigger
  uint8_t mask;      // to which bit of the flag byte I shall react
  uint8_t onoff ;    // type of action:  OFF (00) ON(01) TOGGLE (10) PULSE (11); Pulse length = 2^B, where B = bits [2345], B= (action[i] >>2) &0xf
} action;

/*
if action.node is equal to this node, then the flag word in the telegram is evaluated.
4 flags can be set (bits 1 to bit 4 in the flag byte). Each flag is mapped to a port using the mask byte. 
the port is defined in the port[i] array (up to 4 different ports for the four flags)
Actions are: ON, OFF, TOGGLE Port, PULSE with length 2^B * 0.5 seconds. (needs careful software development!)

Example: Node 15,  Port 7 shall turn on on flag 02, port 7 shall turn off on flag 04. Port 8 shall be toggled on flag 0x08, Port 5 shall be pulsed for 2 seconds on flag 0x10

node = 15;
port = 7;
mask = 0x02;
onoff = 0x1;

node = 15;
port = 7;
mask = 0x04;
onoff = 0x0;

node = 15;
port = 8;
mask = 0x08;
onoff = 0x10;

node = 15;
port = 5;
mask = 0x10;
onoff = 0x3 | (0x02 << 2); // B=2, 2^B = 4 * 0.5s = 2s
*/


// d is just a buffer, since I can't do a new operator, I need to pass enough memory space 
// function not part of the mycodec class. since I only need de-interleaving, I save flash space here
void deinterleave (unsigned char* s, int rows)
{
    int i;
    unsigned char d[rows], bit;
    for (i=0; i<rows; i++) d[i] =0;
	
	for (i=0; i<8*rows; i++)
	{
		bit = (s[i%rows] >> (i/rows)) &0x1;
		bit <<= i%8;
		
		d[i/8] |= bit;  
	}
		
    for (i=0; i<rows; i++) s[i] = d[i]; 
}



// Initialise UART
SoftwareSerial mySerial(RXPIN, TXPIN);

// instance of the Radio Module
RFM69 radio;
Configuration config;
myMAC Mac(radio, config, (uint8_t*) KEY, &mySerial);
Calibration CalMode(config, &mySerial, &Mac, BUILD, (uint8_t*) KEY);


//##############################################################################
#define NUM_ACTIONS 4
action actions[NUM_ACTIONS];


void doaction (action actions[], uint8_t num_actions)
{
    for (uint8_t i=0; i< num_actions; i++)
    {
        if(actions[i].node == Mac.rxpacket.payload[NODEID])
        {
            if (actions[i].mask & Mac.rxpacket.payload[FLAGS] )
            {
                uint8_t act = actions[i].onoff &0x3;
                pinMode(actions[i].port, OUTPUT);
                switch (act)
                {
                    case 0:     // OFF
                        digitalWrite(actions[i].port, 0);
                        break;
                    case 1:     // ON
                        digitalWrite(actions[i].port, 1);
                        break;
                    case 2:     // Toggle
                        digitalWrite(actions[i].port, !digitalRead(actions[i].port));
                        break;
                    case 3:     //Pulse
                        // Tbd
                        digitalWrite(actions[i].port, 1);
                        uint8_t exp = (actions[i].onoff>>2) &0xF;
                        delay(500 * pow(2,exp)); // not ok for long pulse times (2 minutes or so)
                        digitalWrite(actions[i].port, 0);
                        break;
                }   
            }
            break;
        }
    }
}
//##############################################################################


/*****************************************************************************/
/****                         blink                                       ****/
/*****************************************************************************/
void activityLed (unsigned char state, unsigned int time = 0) 
{
  if (config.LedPin) 
  {
    pinMode(config.LedPin, OUTPUT);
    if (time == 0) 
    {
      digitalWrite(config.LedPin, state);
    } 
    else 
    {
      digitalWrite(config.LedPin, state);
      delay(time);
      digitalWrite(config.LedPin, !state);
    }
  }
}

/*****************************************************************************/
/*
     Read VCC by  measuring the 1.1V reference and taking VCC as reference voltage.
     set the reference to Vcc and the measurement to the internal 1.1V reference
*/
/*****************************************************************************/
long readVcc() {
  
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
      ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
      ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high<<8) | low;
  //result = 1125300L / result; // Back-Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  //result = VREF / result;
  return result;
}


float getVcc(long vref)
{
    return (float)vref / readVcc();
}

/**********************************************************************/
void setup() 
{
    // configure device. Normally these data come from the EEPROM
    config.Nodeid = 	    THIS_NODEID;
    config.Networkid =		NETWORKID;
    config.Gatewayid = 		22;
    config.Frequencyband = BAND_868MHZ ;
    config.frequency = 865.0;
    config.TxPower = TXPOWER;
    config.LedPin = LEDpin;
    config.RxPin = RXPIN;
    config.TxPin = TXPIN;
    config.EncryptionEnable=1;
    config.FecEnable =1;
    config.InterleaverEnable =1;
    config.SerialEnable =1;
    config.IsRFM69HW = 1;
    config.PaBoost = 0;
    config.FedvSteps= F_DEV_STEPS; 
    
    /***  INITIALIZE SERIAL PORT ***/  
    pinMode(config.RxPin, INPUT);
    pinMode(config.TxPin, OUTPUT);
    mySerial.begin(SERIAL_BAUD);
    mySerial.println(SKETCHNAME);
    
    pinMode(4, OUTPUT);
       
  
    /***                     ***/
    /***     CALIBRATE?      ***/
    /***                     ***/
    CalMode.configure();

    /***                          ***/
    /***  INITIALIZE RADIO MODULE ***/ 
    /***                          ***/
    mySerial.print("RF Chip = "); config.IsRFM69HW ?    mySerial.print("RFM69HCW") : mySerial.print("RFM69CW");  mySerial.println();
    mySerial.print ("FDEV_STEPS: ");mySerial.print(config.FedvSteps);mySerial.println();
    
    Mac.radio_begin();  // re-initialize radio
    // for debug: this makes sure we can read from the radio.
    // byte version_raw = radio.readReg(REG_VERSION);
    // mySerial.print ("Radio chip ver: "); mySerial.print(version_raw>>4, HEX); mySerial.print (" Radio Metal Mask ver: "); mySerial.print(version_raw&0xf, HEX); mySerial.println();
    
    /*** Test area for taking actions ***/
    
    // turn port 7 ON when flag 0x2 is set in message from Node 15
    actions[0].node = 15;
    actions[0].port = 7;
    actions[0].mask = 0x02;
    actions[0].onoff = 0x1;

    // turn port 7 OFF when flag 0x4 is set in message from Node 15
    actions[1].node = 15;
    actions[1].port = 7;
    actions[1].mask = 0x04;
    actions[1].onoff = 0x0;

    // same for node 24!
    actions[2].node = 24;
    actions[2].port = 7;
    actions[2].mask = 0x02;
    actions[2].onoff = 0x1;

    actions[3].node = 24;
    actions[3].port = 7;
    actions[3].mask = 0x04;
    actions[3].onoff = 0x0; 


 
  if (config.LedPin) 
  {
      activityLed(1, 1000); // LED on
  }
}


void loop()
{
    Mac.radio_receive(true); // blocking
    {
        if (Mac.rxpacket.success || Mac.rxpacket.payload[NODEID] !=0)
        {
            // common for all formats
            mySerial.print("NodeId,"); mySerial.print(Mac.rxpacket.payload[NODEID],DEC);
            mySerial.print(",s=");     mySerial.print(Mac.rxpacket.RSSI);
            mySerial.print(",data:");
            Mac.rxpacket.errorcode >=0 ?    mySerial.print("OK;") : mySerial.print("FAIL;");
            
            // find out which protocol format is used
            if (!(Mac.rxpacket.payload[FLAGS] & 0x20)) // bit 5 in Flags is 0, flags is xx0x xxxx
            {         
                // This is the standard data structure foir Senors or actors
                Payload *pl = (Payload*) Mac.rxpacket.payload;
                mySerial.print(pl->supplyV);   mySerial.print(";");
                mySerial.print(pl->count);     mySerial.print(";");
                mySerial.print((pl->temp - 1000)*4); mySerial.print(";");
                mySerial.print(pl->humidity/2.0); mySerial.print(";");
                mySerial.print(pl->flags, HEX); 
                mySerial.print(";");
               
                        
                if (Mac.rxpacket.errorcode >=0) doaction(actions, NUM_ACTIONS);
            }
            else
            {
                // it is another packet type: work in progress.
                mySerial.print(Mac.rxpacket.payload[TARGETID]); mySerial.print(";");
                mySerial.print(Mac.rxpacket.payload[NODEID]); mySerial.print(";");
                mySerial.print("0x");mySerial.print(Mac.rxpacket.payload[FLAGS],HEX); mySerial.print(";");
                
                switch (Mac.rxpacket.payload[FLAGS] & 0x0f)
                {
                    case 1:
                        // string packet with length 16 (so we've got 13 bytes effective)
                        Mac.rxpacket.payload[16] = 0;
                        mySerial.print((char*)(Mac.rxpacket.payload+3)); mySerial.print(";");
                        break;
                    case 2:
                        // string packet with length 24 (so we've got 21 bytes effective)
                        Mac.rxpacket.payload[24] = 0;
                        mySerial.print((char*)(Mac.rxpacket.payload+3)); mySerial.print(";");
                        break;
                    default:
                        mySerial.print("unknown format!;");
                }
                
            }
            
            mySerial.print(",FEI=");    mySerial.print(Mac.rxpacket.FEI, DEC);
            mySerial.print(",T=");      mySerial.print(int(radio.readTemperature(0)));
            if (config.FecEnable) {mySerial.print(",biterrors=");  mySerial.print(Mac.rxpacket.numerrors);} // error detection only possible with FEC
            mySerial.println(""); 
        }
        else
        {
         /* Errorcodes:
             -1:  could not decode FEC data (too many bit errors in codes)
             -2:  data length does not match
             -3:  not my message or address corrupted
         */
        }    
        //activityLed(1,30); // LED on // commented out because this makes us blind for 30 ms
    }   
}




